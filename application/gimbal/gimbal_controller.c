/*
 * 执行云台 yaw/pitch 控制、重力补偿和耦合补偿。
 * 电机编号和参数来自机器人配置，不在这里拼接 CAN 数据。
 */
#include "gimbal_controller.h"
#include "message_center.h"
#include "motor_driver.h"
#include "motor_service.h"
#include "pid.h"
#include "printing.h"
#include "logger.h"
#include "bsp_time.h"
#include <math.h>
#include <string.h>

// Motor IDs (dynamically assigned during init)
static uint8_t s_pitch_motor_id = 0xFF;
static uint8_t s_yaw_motor_id = 0xFF;

// Yaw control parameters
#define YAW_CONTROL_ENC_MAX (8192.0f)
#define YAW_CONTROL_GYRO_LPF_ALPHA (0.5f)  // Reduced filtering for faster response (was 0.3)
#define CURRENT_LIMIT (30000.0f)

// Gimbal tilt compensation parameters
#define GIMBAL_HEIGHT_CM (30.0f)  // 云台距地面高度 30cm
#define COMPENSATION_UPDATE_RATE_MS (100) // 更新补偿值的频率 100ms

// Legacy defines (not used anymore - values from config)
#define YAW_RPM_MAX (220.0f) * 2.0f
#define YAW_RPM_MIN 0.0f
#define YAW_ERROR_FOR_FULL_SPEED (1200.0f)
// Static state for application
static GimbalCmd s_last_cmd;
static SensorData s_last_sensor;
static bool s_initialized = false;
static bool s_startup_position_captured = false;

/*
 * The yaw/pitch coupling calculation needs two real yaw samples.  Treating the
 * encoder's power-on value as movement from zero would change the pitch target
 * on the first hold command and can make the gimbal jump during calibration.
 */
static bool s_coupling_history_valid = false;
static float s_last_coupling_yaw_angle = 0.0f;
static uint32_t s_last_coupling_yaw_time_ms = 0U;

/*
 * Capture both configured axes as one startup state.  Holding either axis
 * before its first CAN feedback could use a configured angle with a zero
 * encoder value, so the function keeps all gimbal output disabled until every
 * configured axis has reported once.
 */
static bool capture_startup_position(void) {
  MotorContext_t *yaw = MotorDriver_GetContext(s_yaw_motor_id);
  MotorContext_t *pitch = MotorDriver_GetContext(s_pitch_motor_id);

  bool yaw_ready = (s_yaw_motor_id == 0xFF) ||
                   (yaw && yaw->initialized && yaw->last_feedback_time != 0U);
  bool pitch_ready = (s_pitch_motor_id == 0xFF) ||
                     (pitch && pitch->initialized &&
                      pitch->last_feedback_time != 0U);
  if (!yaw_ready || !pitch_ready) {
    return false;
  }

  if (pitch && pitch->config) {
    float pitch_angle = (float)pitch->angle_raw;
    if (pitch_angle < pitch->config->limits.gm6020.angle_min ||
        pitch_angle > pitch->config->limits.gm6020.angle_max) {
      /* Outside the configured safe range: leave both axes unpowered. */
      return false;
    }
  }

  if (yaw) {
    yaw->angle_target = (float)yaw->angle_raw;
    yaw->angle_initialized = true;
    PID_Reset(&yaw->pid_outer);
    PID_Reset(&yaw->pid_inner);
    s_coupling_history_valid = true;
    s_last_coupling_yaw_angle = (float)yaw->angle_raw;
    s_last_coupling_yaw_time_ms = BspTime_NowMs();
  }
  if (pitch) {
    pitch->angle_target = (float)pitch->angle_raw;
    pitch->angle_initialized = true;
    PID_Reset(&pitch->pid_outer);
    PID_Reset(&pitch->pid_inner);
  }

  s_startup_position_captured = true;
  return true;
}

int16_t GimbalController_PitchControl(uint8_t id, float rate_normalized,
                                      SensorData *sensor_data, bool disable_yaw_pitch_compensation) {
  (void)sensor_data; // Not used for pitch
  MotorContext_t *c = MotorDriver_GetContext(id);
  MotorContext_t *yaw = MotorDriver_GetContext(s_yaw_motor_id);
  if (!c || !c->angle_initialized) {
    return 0;
  }

  // Joystick control with sensitivity scaling
  float sensitivity = 60.0f; // Increased for more responsive tracking
  c->angle_target += c->config->direction * sensitivity * rate_normalized;

  // === Pitch补偿：补偿yaw旋转带来的pitch耦合效应 ===
  // 当yaw旋转时，如果pitch有角度，会产生pitch方向的视觉偏移
  // 补偿公式：Δpitch = sin(Δyaw) * tan(pitch)
  // 自瞄时禁用此补偿，因为视觉系统不控制pitch，避免干扰
  if (yaw && yaw->angle_initialized) {
    uint32_t current_time = BspTime_NowMs();
    float current_yaw_angle = (float)yaw->angle_raw;

    if (!s_coupling_history_valid) {
      /*
       * The first valid encoder value is a reference point, not a rotation.
       * Seed the history without applying any pitch correction.
       */
      s_coupling_history_valid = true;
    } else {
      // 计算yaw角度变化（处理0-8192的环绕）
      float yaw_delta = current_yaw_angle - s_last_coupling_yaw_angle;
      if (yaw_delta > 4096.0f) {
        yaw_delta -= 8192.0f;
      } else if (yaw_delta < -4096.0f) {
        yaw_delta += 8192.0f;
      }

      // 只有当yaw有显著变化且时间间隔合理时才计算补偿
      if (!disable_yaw_pitch_compensation && fabsf(yaw_delta) > 1.0f &&
          (current_time - s_last_coupling_yaw_time_ms) > 0U) {
        // 获取当前pitch角度
        float max_encoder = (c->config->limits.gm6020.angle_max > 0.0f)
                                ? c->config->limits.gm6020.angle_max
                                : 8192.0f;
        float current_angle = (float)c->angle_raw;
        float pitch_angle_rad = (current_angle / max_encoder) * (2.0f * (float)M_PI);

        // 将yaw变化转换为弧度
        float yaw_delta_rad = (yaw_delta / 8192.0f) * (2.0f * (float)M_PI);

        // 计算pitch补偿（编码器刻度）
        // Δpitch = sin(Δyaw) * tan(pitch_current)
        float pitch_compensation_rad = sinf(yaw_delta_rad) * tanf(pitch_angle_rad);
        float pitch_compensation_ticks = pitch_compensation_rad * (max_encoder / (2.0f * (float)M_PI));

        // 应用补偿到pitch目标角度（反向补偿以抵消耦合效应）
        c->angle_target -= pitch_compensation_ticks;
      }
    }

    /* Keep the reference current even while compensation is disabled. */
    s_last_coupling_yaw_angle = current_yaw_angle;
    s_last_coupling_yaw_time_ms = current_time;
  }

  // Determine if this is pitch motor (has angle limits)
  bool is_pitch_motor = (c->role == MOTOR_ROLE_GIMBAL_PITCH);
  float max_encoder = (c->config->limits.gm6020.angle_max > 0.0f)
                          ? c->config->limits.gm6020.angle_max
                          : 8192.0f;

  if (is_pitch_motor) {
    if (c->angle_target > c->config->limits.gm6020.angle_max)
      c->angle_target = c->config->limits.gm6020.angle_max;
    if (c->angle_target < c->config->limits.gm6020.angle_min)
      c->angle_target = c->config->limits.gm6020.angle_min;
  } else {
    if (c->angle_target >= max_encoder)
      c->angle_target = c->config->limits.gm6020.angle_min;
    else if (c->angle_target < c->config->limits.gm6020.angle_min)
      c->angle_target = max_encoder;
  }

  float current_angle = (float)c->angle_raw;
  float error = c->angle_target - current_angle;
  if (error > max_encoder / 2.0f)
    error -= max_encoder;
  else if (error < -max_encoder / 2.0f)
    error += max_encoder;

  float cmd = PID_Calculate(&c->pid_outer, error, 0.0f);

  if (is_pitch_motor) {
    float ang01 = current_angle / max_encoder;
    float ang_rad = ang01 * (2.0f * (float)M_PI);
    float gravity_ff = c->config->direction *
                       c->config->limits.gm6020.gravity_compensation *
                       sinf(ang_rad);
    cmd += gravity_ff;
  }
  float max_abs = 25000.0f;
  if (cmd > max_abs)
    cmd = max_abs;
  if (cmd < -max_abs)
    cmd = -max_abs;

  // Pitch PID tuning CSV (20Hz rate limited in main.c) - DISABLED for clean output
  // Format: GIM,timestamp_ms,angle_target,angle_current,speed_rpm,cmd,error,rate_scaled
  // LOG_CSV(LOG_TAG_GIM, "PITCH,%.2f,%.2f,%d,%.2f,%.2f,%.2f",
  //         c->angle_target,
  //         current_angle,
  //         c->speed_rpm,
  //         cmd,
  //         error,
  //         rate_normalized * 300.0f);

  return (int16_t)cmd;
}

// Test mode: generates step signal for tuning
#define YAW_TEST_MODE 0

#if YAW_TEST_MODE
static int16_t test_counter = 0;
static int16_t test_target = 0;
#endif

int16_t GimbalController_YawControlWithCompensation(float rate_normalized,
                                                    SensorData *sensor_data,
                                                    bool use_imu_feedback) {
  MotorContext_t *yaw = MotorDriver_GetContext(s_yaw_motor_id);
  if (!yaw || !yaw->angle_initialized)
    return 0;

#if YAW_TEST_MODE
  // Generate step signal for testing (full 360° rotation over 18 seconds)
  // Increments by 8192/18 ≈ 455 ticks every 1 second (200 cycles @ 5ms)
  if (test_counter++ % 200 == 0) {
    test_target += 8192 / 18;
    if (test_target >= 8192)
      test_target = 0;

    // Reset PID on target jump to prevent integral windup
    PID_Reset(&yaw->pid_outer);
    PID_Reset(&yaw->pid_inner);
  }
  yaw->angle_target = (float)test_target;
#else
  // Joystick control (increased sensitivity for more responsive tracking)
  yaw->angle_target += 70.0f * rate_normalized;
#endif

  // Wrap target into encoder range
  if (yaw->angle_target >= YAW_CONTROL_ENC_MAX)
    yaw->angle_target -= YAW_CONTROL_ENC_MAX;
  else if (yaw->angle_target < 0)
    yaw->angle_target += YAW_CONTROL_ENC_MAX;

  float current = yaw->angle_raw;
  float angle_error = yaw->angle_target - current;

  // Ultra-minimal deadband for maximum tracking precision (reduced from 1.0 → 0.1 → 0.02)
  if (fabsf(angle_error) < 0.02f)
    angle_error = 0.0f;

  // wrap error into [-ENC_MAX/2, ENC_MAX/2]
  if (angle_error > YAW_CONTROL_ENC_MAX / 2.0f)
    angle_error -= YAW_CONTROL_ENC_MAX;
  if (angle_error < -YAW_CONTROL_ENC_MAX / 2.0f)
    angle_error += YAW_CONTROL_ENC_MAX;

  // ==========================
  // OUTER LOOP: angle → speed
  // ==========================
  float cmd_angle_to_speed = PID_Calculate(&yaw->pid_outer, 0.0f, -angle_error);

  // Dynamic speed limit based on control mode
  // Auto-aim mode (rate_normalized == 0.0f): Higher speed for fast target tracking
  // Manual joystick mode: Standard speed for smooth control
  float rpm_limit;
  if (rate_normalized == 0.0f) {
    // Auto-aim/Spin-hold mode: High-speed tracking (2778°/s ≈ 7.72 rot/s)
    rpm_limit = 500.0f;
  } else {
    // Manual joystick mode: Standard speed (1667°/s ≈ 4.63 rot/s)
    rpm_limit = 300.0f;
  }

  // Apply signed clamp
  if (cmd_angle_to_speed > rpm_limit)
    cmd_angle_to_speed = rpm_limit;
  if (cmd_angle_to_speed < -rpm_limit)
    cmd_angle_to_speed = -rpm_limit;

  // Speed feedback source selection:
  // - Spin mode: Use IMU gyro for absolute yaw stability
  // - Normal mode: Use motor encoder for better response
  float speed_feedback;
  if (use_imu_feedback) {
    // IMU gyro feedback (for spin mode)
    // Convert IMU gyro (rad/s) to RPM: 1 rad/s = 30/π RPM ≈ 9.549 RPM
    speed_feedback = -sensor_data->g_gz * 30.0f /
                     (float)M_PI; // negative because IMU z-axis convention
  } else {
    // Motor encoder speed feedback (for normal mode)
    speed_feedback = (float)yaw->speed_rpm;
  }

  float cmd_speed_to_current =
      PID_Calculate(&yaw->pid_inner, cmd_angle_to_speed, speed_feedback);

  // Clamp current
  if (cmd_speed_to_current > CURRENT_LIMIT)
    cmd_speed_to_current = CURRENT_LIMIT;
  if (cmd_speed_to_current < -CURRENT_LIMIT)
    cmd_speed_to_current = -CURRENT_LIMIT;

  // Yaw PID tuning CSV (20Hz rate limited)
  // Format: YAW_CSV,timestamp_ms,target_angle,current_angle,speed_rpm,cmd_current,cmd_speed,rate,error,g_gz_filtered,c_gz
  // LOG_CSV(LOG_TAG_GIM, "YAW_CSV,%.2f,%.2f,%d,%.2f,%.4f,%.4f,%.4f,%.2f,%.4f,%.4f",
  //         yaw->angle_target,
  //         current,
  //         yaw->speed_rpm,
  //         cmd_speed_to_current,
  //         cmd_angle_to_speed,
  //         rate_normalized * 300.0f,
  //         angle_error,
  //         sensor_data->g_gz * YAW_CONTROL_GYRO_LPF_ALPHA +
  //             s_last_sensor.g_gz * (1.0f - YAW_CONTROL_GYRO_LPF_ALPHA),
  //         sensor_data->c_gz);

  return (int16_t)cmd_speed_to_current;
}

/**
 * @brief 计算并显示云台倾斜角度补偿值
 *
 * 当云台pitch轴倾斜时，为了锁定地面目标，需要计算yaw角度补偿
 * 补偿量 = atan(sin(yaw_delta) * tan(pitch))
 *
 * 物理模型：
 * - 云台高度：30cm
 * - pitch角度：θ_p (正值向上)
 * - yaw旋转：θ_y
 * - 当yaw旋转时，由于pitch不为0，会产生垂直方向的指向偏移
 */
void GimbalController_CalculateAndDisplayCompensation(void) {
  static uint32_t last_update_time = 0;
  uint32_t current_time = BspTime_NowMs();

  // 限制更新频率，避免刷屏
  if (current_time - last_update_time < COMPENSATION_UPDATE_RATE_MS) {
    return;
  }
  last_update_time = current_time;

  // 获取pitch和yaw电机上下文
  MotorContext_t *pitch = MotorDriver_GetContext(s_pitch_motor_id);
  MotorContext_t *yaw = MotorDriver_GetContext(s_yaw_motor_id);

  if (!pitch || !pitch->angle_initialized || !yaw || !yaw->angle_initialized) {
    return;
  }

  // 计算当前pitch角度（弧度）
  float max_encoder_pitch = (pitch->config->limits.gm6020.angle_max > 0.0f)
                              ? pitch->config->limits.gm6020.angle_max
                              : 8192.0f;
  float pitch_angle_normalized = (float)pitch->angle_raw / max_encoder_pitch;
  float pitch_angle_rad = pitch_angle_normalized * (2.0f * (float)M_PI);
  float pitch_angle_deg = pitch_angle_rad * 180.0f / (float)M_PI;

  // 计算当前yaw角度（度）
  float max_encoder_yaw = (yaw->config->limits.gm6020.angle_max > 0.0f)
                            ? yaw->config->limits.gm6020.angle_max
                            : 8192.0f;
  float yaw_angle_deg = ((float)yaw->angle_raw / max_encoder_yaw) * 360.0f;

  // 计算目标水平距离（假设目标在地面上）
  // tan(pitch) = height / distance
  // distance = height / tan(pitch)
  float target_distance_cm = 0.0f;
  float pitch_for_distance = pitch_angle_rad;

  if (fabsf(pitch_for_distance) > 0.01f) { // 避免除零
    target_distance_cm = GIMBAL_HEIGHT_CM / tanf(pitch_for_distance);
  }

  // 计算yaw旋转1度时的补偿量
  // 当pitch不为0时，yaw旋转会导致俯仰角度的视觉偏移
  // 补偿公式：Δpitch ≈ sin(Δyaw) * tan(pitch)
  float yaw_delta_rad = 1.0f * (float)M_PI / 180.0f; // 1度
  float pitch_compensation_rad = sinf(yaw_delta_rad) * tanf(pitch_angle_rad);
  float pitch_compensation_deg = pitch_compensation_rad * 180.0f / (float)M_PI;

  // 计算yaw补偿（用于保持指向同一目标）
  // 当pitch角度改变时，如果要保持指向相同水平距离的点
  // yaw角度需要微调
  float yaw_compensation_per_pitch_deg = 0.0f;
  if (fabsf(cosf(pitch_angle_rad)) > 0.01f) {
    // Δyaw ≈ sin(pitch) * Δpitch / cos(pitch)
    yaw_compensation_per_pitch_deg = sinf(pitch_angle_rad) / cosf(pitch_angle_rad);
  }

  // 显示补偿信息
  USB_CDC_Printf("\r\n=== 云台角度补偿计算 ===\r\n");
  USB_CDC_Printf("云台高度: %.1f cm\r\n", GIMBAL_HEIGHT_CM);
  USB_CDC_Printf("当前Pitch角度: %.2f° (%.4f rad)\r\n", pitch_angle_deg, pitch_angle_rad);
  USB_CDC_Printf("当前Yaw角度: %.2f°\r\n", yaw_angle_deg);

  // 显示pitch控制模式
  if (s_last_cmd.vision_valid) {
    USB_CDC_Printf("Pitch控制: 遥控器手动 (自瞄时视觉不控制pitch)\r\n");
  } else {
    USB_CDC_Printf("Pitch控制: 遥控器手动 + Yaw-Pitch耦合补偿\r\n");
  }

  if (fabsf(pitch_for_distance) > 0.01f && target_distance_cm > 0.0f) {
    USB_CDC_Printf("目标水平距离: %.1f cm\r\n", target_distance_cm);
  } else if (target_distance_cm < 0.0f) {
    USB_CDC_Printf("目标水平距离: %.1f cm (目标在云台后方)\r\n", -target_distance_cm);
  } else {
    USB_CDC_Printf("目标水平距离: 无穷远 (pitch≈0°)\r\n");
  }

  USB_CDC_Printf("\r\n补偿值:\r\n");
  USB_CDC_Printf("- Yaw旋转1°时的Pitch耦合: %.4f° (%.6f rad)\r\n",
                 pitch_compensation_deg, pitch_compensation_rad);
  USB_CDC_Printf("- Pitch变化1°时需要的Yaw补偿系数: %.4f\r\n",
                 yaw_compensation_per_pitch_deg);

  // 计算实际补偿到编码器刻度
  float pitch_comp_ticks = pitch_compensation_rad * (max_encoder_pitch / (2.0f * (float)M_PI));
  USB_CDC_Printf("- Yaw旋转1°的Pitch补偿(编码器刻度): %.2f ticks\r\n", pitch_comp_ticks);

  USB_CDC_Printf("========================\r\n\r\n");

  // 记录到日志（CSV格式，便于后续分析）
  LOG_CSV(LOG_TAG_GIM, "COMPENSATION,%.2f,%.2f,%.1f,%.4f,%.4f,%.2f",
          pitch_angle_deg,
          yaw_angle_deg,
          target_distance_cm,
          pitch_compensation_deg,
          yaw_compensation_per_pitch_deg,
          pitch_comp_ticks);
}

// Application layer: Message subscription callbacks
static void on_gimbal_cmd(const MsgEvent *ev, void *user) {
  (void)user;
  if (ev->size == sizeof(GimbalCmd)) {
    memcpy(&s_last_cmd, ev->data, sizeof(GimbalCmd));

    /*
     * A command may arrive after the startup wait timed out. Capture late CAN
     * feedback here as well, but never energize a configured axis beforehand.
     */
    bool startup_ready = s_startup_position_captured ||
                         capture_startup_position();

    // Execute gimbal control when command arrives and both axes are safe.
    if (s_last_cmd.enabled && startup_ready) {
      static bool s_yaw_vision_active = false;
      static bool s_yaw_spin_hold_active = false;
      bool use_vision_target = s_last_cmd.vision_valid;
      bool use_spin_hold =
          (!use_vision_target) && (s_last_cmd.yaw_rate_memo > 0.5f);

      // Continuous angle control: update target angle every cycle when vision
      // is valid
      if (use_vision_target) {
        MotorContext_t *yaw = MotorDriver_GetContext(s_yaw_motor_id);
        MotorContext_t *pitch = MotorDriver_GetContext(s_pitch_motor_id);

        if (yaw && yaw->angle_initialized) {
          float max_encoder = (yaw->config->limits.gm6020.angle_max > 0.0f)
                                  ? yaw->config->limits.gm6020.angle_max
                                  : 8192.0f;
          const float ticks_per_rad = max_encoder / (2.0f * (float)M_PI);
          float err_ticks = s_last_cmd.vision_yaw_err_rad * ticks_per_rad;
          // Update target angle continuously based on current angle + vision
          // error
          yaw->angle_target = (float)yaw->angle_raw + err_ticks;
          while (yaw->angle_target >= max_encoder)
            yaw->angle_target -= max_encoder;
          while (yaw->angle_target < 0.0f)
            yaw->angle_target += max_encoder;

          if (!s_yaw_vision_active) {
            PID_Reset(&yaw->pid_outer);
            PID_Reset(&yaw->pid_inner);
          }
          s_yaw_vision_active = true;
        }

        (void)pitch;
      } else {
        s_yaw_vision_active = false;
      }

      // Spin-hold mode: hold gimbal absolute yaw (deg) using gimbal IMU
      // yaw_total_angle. Target is carried via yaw_target_memo; enable flag via
      // yaw_rate_memo.
      if (use_spin_hold) {
        MotorContext_t *yaw = MotorDriver_GetContext(s_yaw_motor_id);
        if (yaw && yaw->angle_initialized) {
          float max_encoder = (yaw->config->limits.gm6020.angle_max > 0.0f)
                                  ? yaw->config->limits.gm6020.angle_max
                                  : 8192.0f;

          // Calculate angle error with proper wrapping to [-180, 180] range
          float yaw_err_deg =
              s_last_cmd.yaw_target_memo - s_last_sensor.yaw_total_angle;

          // Wrap error to shortest path
          while (yaw_err_deg > 180.0f)
            yaw_err_deg -= 360.0f;
          while (yaw_err_deg < -180.0f)
            yaw_err_deg += 360.0f;

          // Convert to encoder ticks
          float ticks_per_deg = max_encoder / 360.0f;
          float err_ticks = yaw_err_deg * ticks_per_deg;

          // Set target angle in encoder space
          yaw->angle_target = (float)yaw->angle_raw + err_ticks;
          while (yaw->angle_target >= max_encoder)
            yaw->angle_target -= max_encoder;
          while (yaw->angle_target < 0.0f)
            yaw->angle_target += max_encoder;

          if (!s_yaw_spin_hold_active) {
            PID_Reset(&yaw->pid_outer);
            PID_Reset(&yaw->pid_inner);
          }
          s_yaw_spin_hold_active = true;
        } else {
          s_yaw_spin_hold_active = false;
        }
      } else {
        s_yaw_spin_hold_active = false;
      }

      int16_t pitch_current = GimbalController_PitchControl(
          s_pitch_motor_id,
          s_last_cmd.pitch_rate,  // 遥控器始终可以控制pitch
          &s_last_sensor,
          use_vision_target  // 自瞄时禁用yaw-pitch耦合补偿
      );
      int16_t yaw_current = GimbalController_YawControlWithCompensation(
          (use_vision_target || use_spin_hold) ? 0.0f : s_last_cmd.yaw_rate,
          &s_last_sensor,
          use_spin_hold // Use IMU feedback only in spin mode
      );

      // 计算并显示云台倾斜角度补偿（每100ms更新一次）
      GimbalController_CalculateAndDisplayCompensation();

      // Send motor currents (buffered, will be flushed by message center)
      (void)MotorService_CommandCurrent(s_pitch_motor_id, pitch_current);
      (void)MotorService_CommandCurrent(s_yaw_motor_id, yaw_current);
    } else {
      // Disabled or missing safe startup feedback: keep both outputs at zero.
      (void)MotorService_CommandCurrent(s_pitch_motor_id, 0);
      (void)MotorService_CommandCurrent(s_yaw_motor_id, 0);
    }

    // Gimbal position logging (20Hz rate limited in main.c)
    // Format: GIM,timestamp_ms,ENCODER,yaw_raw,pitch_raw,yaw_tgt,pitch_tgt
    // DISABLED to reduce log clutter - use only YAW_CSV for plotting
    // MotorContext_t *yaw_ctx = MotorDriver_GetContext(s_yaw_motor_id);
    // MotorContext_t *pitch_ctx = MotorDriver_GetContext(s_pitch_motor_id);

    // Support both full gimbal (yaw+pitch) and yaw-only configurations
    // if (yaw_ctx && pitch_ctx) {
    //   // Both yaw and pitch exist
    //   LOG_CSV(LOG_TAG_GIM, "ENCODER,%.2f,%.2f,%.2f,%.2f",
    //           (float)yaw_ctx->angle_raw,
    //           (float)pitch_ctx->angle_raw,
    //           yaw_ctx->angle_target,
    //           pitch_ctx->angle_target);
    // } else if (yaw_ctx) {
    //   // Yaw-only configuration (e.g., sentry)
    //   LOG_CSV(LOG_TAG_GIM, "ENCODER,%.2f,0.0,%.2f,0.0",
    //           (float)yaw_ctx->angle_raw,
    //           yaw_ctx->angle_target);
    // } else if (pitch_ctx) {
    //   // Pitch-only configuration (unlikely but handle it)
    //   LOG_CSV(LOG_TAG_GIM, "ENCODER,0.0,%.2f,0.0,%.2f",
    //           (float)pitch_ctx->angle_raw,
    //           pitch_ctx->angle_target);
    // }
  }
}

static void on_imu_update(const MsgEvent *ev, void *user) {
  (void)user;
  if (ev->size == sizeof(SensorData)) {
    memcpy(&s_last_sensor, ev->data, sizeof(SensorData));
  }
}

void GimbalApp_Init(void) {
  if (s_initialized) {
    return;
  }

  memset(&s_last_cmd, 0, sizeof(s_last_cmd));
  memset(&s_last_sensor, 0, sizeof(s_last_sensor));
  s_startup_position_captured = false;
  s_coupling_history_valid = false;
  s_last_coupling_yaw_angle = 0.0f;
  s_last_coupling_yaw_time_ms = 0U;

  LOG_INFO(LOG_TAG_GIM, "Gimbal init: searching for motors...");

  // Find gimbal motors by role (module layer handles config)
  uint8_t pitch_motors[1];
  uint8_t yaw_motors[1];

  if (MotorService_FindByRole(MOTOR_ROLE_GIMBAL_PITCH, pitch_motors, 1) > 0) {
    s_pitch_motor_id = pitch_motors[0];
    LOG_INFO(LOG_TAG_GIM, "Found pitch motor: id=%d", s_pitch_motor_id);
  } else {
    LOG_INFO(LOG_TAG_GIM, "No pitch motor configured");
  }

  if (MotorService_FindByRole(MOTOR_ROLE_GIMBAL_YAW, yaw_motors, 1) > 0) {
    s_yaw_motor_id = yaw_motors[0];
    LOG_INFO(LOG_TAG_GIM, "Found yaw motor: id=%d", s_yaw_motor_id);
  } else {
    LOG_ERROR(LOG_TAG_GIM, "ERROR: No yaw motor found!");
  }

  // Subscribe to messages
  (void)MsgCenter_Subscribe(TOPIC_GIMBAL_CMD, on_gimbal_cmd, NULL);
  (void)MsgCenter_Subscribe(TOPIC_IMU_UPDATE, on_imu_update, NULL);

  LOG_INFO(LOG_TAG_GIM, "Gimbal init complete: yaw=%d pitch=%d", s_yaw_motor_id, s_pitch_motor_id);
  s_initialized = true;
}

/**
 * @brief Wait for both axes to report feedback and hold their power-on position
 * @note A zero-rate command does not select a new mechanical angle. Both axes
 *       capture their first encoder feedback before producing holding current.
 */
void Gimbal_WaitForAlignment(void) {
  const uint32_t TIMEOUT_MS = 5000;        // 5 seconds timeout (reduced from 10s)
  const uint32_t CHECK_INTERVAL_MS = 5;    // 5ms check (reduced from 100ms → 20x faster)

  USB_CDC_Printf("[Gimbal] Waiting for startup position feedback...\r\n");

  // Enable zero-rate holding after each motor has captured its first feedback.
  GimbalCmd cmd = {.enabled = true,
                   .pitch_rate = 0.0f,
                   .yaw_rate = 0.0f,
                   .yaw_rate_memo = 0.0f,
                   .yaw_target_memo = 0.0f,
                   .vision_valid = false,
                   .vision_yaw_err_rad = 0.0f,
                   .vision_pitch_err_rad = 0.0f,
                   .vision_ts_ms = 0};

  uint32_t start_time = BspTime_NowMs();

  while (BspTime_NowMs() - start_time < TIMEOUT_MS) {
    /* Process queued motor feedback before any non-zero holding command. */
    MsgCenter_Dispatch();

    if (s_startup_position_captured || capture_startup_position()) {
      (void)MsgCenter_Publish(TOPIC_GIMBAL_CMD, &cmd, sizeof(cmd));
      MsgCenter_Dispatch();
      USB_CDC_Printf("[Gimbal] Startup position captured!\r\n");
      return;
    }

    BspTime_DelayMs(CHECK_INTERVAL_MS);
  }

  USB_CDC_Printf(
      "[Gimbal] Warning: Startup feedback timeout; output remains zero until feedback arrives.\r\n");
}
