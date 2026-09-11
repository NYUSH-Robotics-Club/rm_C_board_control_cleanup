/*
 * 保存步兵全向轮机器人的几何、电机、方向、限幅和控制参数。
 * 这些值会直接影响实车，修改后必须重新编译并上板验证。
 */
#include "infantry_standard.h"

/**
 * @brief Standard Infantry Robot Configuration
 *
 * - 4x M3508 chassis motors (omni wheels)
 * - 2x GM6020 gimbal motors (pitch + yaw)
 * - 3x M3508 shooter motors (turntable + 2x friction wheels)
 */

/*
 * motor_id 是全局唯一的软件编号；重复编号会使电机服务拒绝初始化。
 * 硬件 ID 由电机/电调设置，下面的 CAN 地址和槽位与它对应，不会修改硬件 ID。
 * CAN1：底盘硬件 ID 1~4、Yaw ID 5；CAN2：左右摩擦轮 ID 1/2、拨弹 ID 3、Pitch ID 4。
 * tx_slot 从 0 开始，每槽占控制报文的两个字节，高字节在前。
 */
static const MotorConfig_t g_motor_configs_infantry_standard[] = {
    // ========== CHASSIS MOTORS (4x M3508) ==========
    // 右后轮：CAN1 硬件 ID 1，软件编号 1。
    {
        .motor_id = 1,
        .vendor = MOTOR_VENDOR_DJI,
        .type = MOTOR_TYPE_M3508,
        .role = MOTOR_ROLE_CHASSIS_DRIVE,
        .control_mode = MOTOR_CONTROL_APPLICATION,
        .can_channel = CAN_CHANNEL_1,
        .can_rx_id = 0x201,
        .can_tx_id = 0x200,
        .tx_slot = 0,
        .direction = -1, // 旧安装方向值；全向轮启用前须核对正转方向。
        .limits.m3508 = {.speed_limit = 10000.0f},
        .pid_outer = {5.0f, 0.0f, 0.01f, 5000.0f, 7500.0f}, // Speed PID
        .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}          // Not used
    },

    // 右前轮：CAN1 硬件 ID 2，软件编号 2。
    {.motor_id = 2,
     .vendor = MOTOR_VENDOR_DJI,
     .type = MOTOR_TYPE_M3508,
     .role = MOTOR_ROLE_CHASSIS_DRIVE,
     .control_mode = MOTOR_CONTROL_APPLICATION,
     .can_channel = CAN_CHANNEL_1,
     .can_rx_id = 0x202,
     .can_tx_id = 0x200,
     .tx_slot = 1,
     .direction = +1,
     .limits.m3508 = {.speed_limit = 10000.0f},
     .pid_outer = {5.0f, 0.0f, 0.01f, 5000.0f, 7500.0f},
     .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // 左前轮：CAN1 硬件 ID 3，软件编号 3。
    {.motor_id = 3,
     .vendor = MOTOR_VENDOR_DJI,
     .type = MOTOR_TYPE_M3508,
     .role = MOTOR_ROLE_CHASSIS_DRIVE,
     .control_mode = MOTOR_CONTROL_APPLICATION,
     .can_channel = CAN_CHANNEL_1,
     .can_rx_id = 0x203,
     .can_tx_id = 0x200,
     .tx_slot = 2,
     .direction = 1,
     .limits.m3508 = {.speed_limit = 10000.0f},
     .pid_outer = {5.0f, 0.0f, 0.01f, 5000.0f, 7500.0f},
     .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // 左后轮：CAN1 硬件 ID 4，软件编号 4。
    {.motor_id = 4,
     .vendor = MOTOR_VENDOR_DJI,
     .type = MOTOR_TYPE_M3508,
     .role = MOTOR_ROLE_CHASSIS_DRIVE,
     .control_mode = MOTOR_CONTROL_APPLICATION,
     .can_channel = CAN_CHANNEL_1,
     .can_rx_id = 0x204,
     .can_tx_id = 0x200,
     .tx_slot = 3,
     .direction = -1,
     .limits.m3508 = {.speed_limit = 10000.0f},
     .pid_outer = {5.0f, 0.0f, 0.01f, 5000.0f, 7500.0f},
     .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // ========== SHOOTER MOTORS (3x M3508) ==========
    // 拨弹：CAN2 硬件 ID 3，软件编号 9。
    {.motor_id = 9,
     .vendor = MOTOR_VENDOR_DJI,
     .type = MOTOR_TYPE_M3508,
     .role = MOTOR_ROLE_SHOOTER_FEED,
     .control_mode = MOTOR_CONTROL_APPLICATION,
     .can_channel = CAN_CHANNEL_2,
     .can_rx_id = 0x203,
     .can_tx_id = 0x200,
     .tx_slot = 2,
     .direction = +1,
     .limits.m3508 = {.speed_limit = 10000.0f},
     .pid_outer =
         {1.0f, 0.0f, 0.0f, 15000.0f,
          7500.0f}, // Shooter feed PID (reduced Kp to 1.2, increased Kd to 0.3
                    // to suppress high-frequency oscillation)
     .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // 左摩擦轮：CAN2 硬件 ID 1，软件编号 6。
    {.motor_id = 6,
     .vendor = MOTOR_VENDOR_DJI,
     .type = MOTOR_TYPE_M3508,
     .role = MOTOR_ROLE_SHOOTER_FRICTION,
     .control_mode = MOTOR_CONTROL_APPLICATION,
     .can_channel = CAN_CHANNEL_2,
     .can_rx_id = 0x201,
     .can_tx_id = 0x200,
     .tx_slot = 0,
     .direction = +1,
     .limits.m3508 = {.speed_limit = 10000.0f},
     .pid_outer = {5.0f, 0.5f, 0.05f, 15000.0f, 7500.0f},
     .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // 右摩擦轮：CAN2 硬件 ID 2，软件编号 7。
    {.motor_id = 7,
     .vendor = MOTOR_VENDOR_DJI,
     .type = MOTOR_TYPE_M3508,
     .role = MOTOR_ROLE_SHOOTER_FRICTION,
     .control_mode = MOTOR_CONTROL_APPLICATION,
     .can_channel = CAN_CHANNEL_2,
     .can_rx_id = 0x202,
     .can_tx_id = 0x200,
     .tx_slot = 1,
     .direction = +1,
     .limits.m3508 = {.speed_limit = 10000.0f},
     .pid_outer = {5.0f, 0.5f, 0.05f, 15000.0f, 7500.0f},
     .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // ========== GIMBAL MOTORS (2x GM6020) ==========
    // Yaw：CAN1 硬件 ID 5，软件编号 5。
    {
        .motor_id = 5,
        .vendor = MOTOR_VENDOR_DJI,
        .type = MOTOR_TYPE_GM6020,
        .role = MOTOR_ROLE_GIMBAL_YAW,
        .control_mode = MOTOR_CONTROL_APPLICATION,
        .can_channel = CAN_CHANNEL_1,
        .can_rx_id = 0x209, // GM6020 反馈地址 = 0x204 + 硬件 ID 5。
        .can_tx_id = 0x2FE, // GM6020 ID5 电流指令，匹配已开启的电调电流环。
        .tx_slot = 0,
        .direction = +1,
        .limits.gm6020 =
            {
                .angle_min = 0.0f,
                .angle_max = 8192.0f,
                .gravity_compensation = 0.0f,
                .initial_angle = -1.0f // Auto-initialize from current position (no startup vibration)
            },
        .protocol.dji = {GM6020_COMMAND_CURRENT, 5460}, // 电流原始刻度，5460 约为 1 A。
        // 角度误差单位为编码器刻度，输出为 RPM；初调限30 RPMjust ，避免小误差即满速换向。
        .pid_outer = {0.0f, 0.0f, 0.0f, 50000.0f, 0.0f},
        // 速度误差单位为 RPM，输出为原始电流；先禁用积分/微分，反馈失联仍归零。
        .pid_inner = {3.3f, 3.3f, 0.825f, 20000.0f, 2000.0f}
    },

    // Pitch：CAN2 硬件 ID 4，软件编号 8。
    {
        .motor_id = 8,
        .vendor = MOTOR_VENDOR_DJI,
        .type = MOTOR_TYPE_GM6020,
        .role = MOTOR_ROLE_GIMBAL_PITCH,
        .control_mode = MOTOR_CONTROL_APPLICATION,
        .can_channel = CAN_CHANNEL_2,
        .can_rx_id = 0x208, // GM6020 反馈地址 = 0x204 + 硬件 ID 4。
        .can_tx_id = 0x1FF, // GM6020 硬件 ID 1~4 的控制报文。
        .tx_slot = 3,
        .direction = -1, // Pitch direction correction
        .limits.gm6020 =
            {
                .angle_min = 1000.0f,
                .angle_max = 4000.0f,
                .gravity_compensation =
                    5000.0f,             // Gravity compensation for pitch
                // Capture the real power-on angle before enabling position hold.
                .initial_angle = -1.0f,
                // 重装后暂停旧机械限位；省略此项会恢复限位，越界时两轴都无法启动。
                .angle_limits_disabled = true
            },
        .pid_outer = {5.0f, 0.0f, 0.1f, 10000.0f, 15000.0f}, // Pitch PID (aggressive: high Kp, low Kd for fast tracking)
        .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f} // Not used for pitch
    }};

/* 用户确认：X形±45°，前后/左右轮中心距均0.54m，轮半径0.07m，M3508 P19。
 * 正驱动向量统一取向前分量为正；电机正反向另由MotorConfig.direction修正。
 * P19手册减速比3591/187；速度是初调上限，首次架空核对四轮方向后再落地。
 */
#define OMNI_HALF_TRACK_M 0.27f
#define OMNI_HALF_WHEELBASE_M 0.27f
#define OMNI_DIAGONAL_UNIT 0.70710678118655f
#define OMNI_RADIUS_M 0.07f
#define M3508_P19_REDUCTION (3591.0f / 187.0f)

static const OmniChassisConfig g_omni_infantry = {
    .configured = 1,
    .max_translation_mps = 0.90f,
    .max_rotation_radps = 1.20f,
    .wheels = {
        {2,  OMNI_HALF_WHEELBASE_M,  OMNI_HALF_TRACK_M,
             OMNI_DIAGONAL_UNIT, -OMNI_DIAGONAL_UNIT, OMNI_RADIUS_M, M3508_P19_REDUCTION},
        {1,  OMNI_HALF_WHEELBASE_M, -OMNI_HALF_TRACK_M,
             OMNI_DIAGONAL_UNIT,  OMNI_DIAGONAL_UNIT, OMNI_RADIUS_M, M3508_P19_REDUCTION},
        {4, -OMNI_HALF_WHEELBASE_M, -OMNI_HALF_TRACK_M,
             OMNI_DIAGONAL_UNIT, -OMNI_DIAGONAL_UNIT, OMNI_RADIUS_M, M3508_P19_REDUCTION},
        {3, -OMNI_HALF_WHEELBASE_M,  OMNI_HALF_TRACK_M,
             OMNI_DIAGONAL_UNIT,  OMNI_DIAGONAL_UNIT, OMNI_RADIUS_M, M3508_P19_REDUCTION},
    }
};

// Robot configuration structure
const RobotConfig_t g_robot_config_infantry_standard = {
    .name = "Infantry Standard",
    .chassis_type = CHASSIS_TYPE_OMNI,
    .omni = &g_omni_infantry,
    .chassis_motor_count = 4,
    .gimbal_motor_count = 2,
    .shooter_motor_count = 3,
    .motor_configs = g_motor_configs_infantry_standard,
    .total_motor_count = 9,
    .enable_imu_calibration = 1 // Infantry needs IMU calibration
};
