/*
 * 接收发射命令并控制拨盘和摩擦轮。
 * 电机选择来自角色配置，停止或反馈超时时输出零电流。
 */
#include "shooter_controller.h"
#include "motor_service.h"
#include <string.h>
#include "message_center.h"
#include "motor_messages.h"
#include "sensor_messages.h"
#include "control_messages.h"
#include "bsp_time.h"

#define MOTOR_FEEDBACK_TIMEOUT_MS (100U)

// Static variables for app wrapper
static ShootCmd s_last_cmd;
static SensorData s_last_sensor;
static ShooterController s_ctrl;

// Shooter motor configuration (dynamically assigned during init)
static uint8_t s_feed_motor_id = 0xFF;      // Turntable/feed motor
static uint8_t s_friction1_motor_id = 0xFF; // Friction wheel 1
static uint8_t s_friction2_motor_id = 0xFF; // Friction wheel 2

static float RampTowards(float current, float target, float step)
{
    if (current < target) { current += step; if (current > target) current = target; }
    else if (current > target) { current -= step; if (current < target) current = target; }
    return current;
}

static int16_t ComputeSingleMotorCurrent(PID_Controller *pid, float target, Motor_Feedback *feedback, uint32_t current_tick)
{
    if (current_tick - feedback->last_update_time > MOTOR_FEEDBACK_TIMEOUT_MS) { return 0; }
    float current_speed = feedback->speed;
    return (int16_t)PID_Calculate(pid, target, current_speed);
}

void ShooterController_Init(ShooterController *controller)
{
    if (controller == NULL) return;
    memset(controller, 0, sizeof(ShooterController));

    // Find shooter motors by role (module layer handles config)
    uint8_t feed_motors[1];
    uint8_t friction_motors[2];

    // Find feed motor
    if (MotorService_FindByRole(MOTOR_ROLE_SHOOTER_FEED, feed_motors, 1) > 0) {
        s_feed_motor_id = feed_motors[0];

        // Initialize turntable PID from motor configuration
        const MotorConfig_t *config = MotorService_GetConfig(s_feed_motor_id);
        if (config) {
            PID_Init(&controller->turntable_pid,
                     config->pid_outer.kp,
                     config->pid_outer.ki,
                     config->pid_outer.kd,
                     config->pid_outer.output_max,
                     config->pid_outer.integral_max);
        }
    }

    // Find friction wheels
    uint8_t friction_count = MotorService_FindByRole(MOTOR_ROLE_SHOOTER_FRICTION, friction_motors, 2);
    if (friction_count > 0) {
        s_friction1_motor_id = friction_motors[0];

        // Initialize shooter1 PID from motor configuration
        const MotorConfig_t *config = MotorService_GetConfig(s_friction1_motor_id);
        if (config) {
            PID_Init(&controller->shooter1_pid,
                     config->pid_outer.kp,
                     config->pid_outer.ki,
                     config->pid_outer.kd,
                     config->pid_outer.output_max,
                     config->pid_outer.integral_max);
        }
    }

    if (friction_count > 1) {
        s_friction2_motor_id = friction_motors[1];

        // Initialize shooter2 PID from motor configuration
        const MotorConfig_t *config = MotorService_GetConfig(s_friction2_motor_id);
        if (config) {
            PID_Init(&controller->shooter2_pid,
                     config->pid_outer.kp,
                     config->pid_outer.ki,
                     config->pid_outer.kd,
                     config->pid_outer.output_max,
                     config->pid_outer.integral_max);
        }
    }
}

void ShooterController_Update(ShooterController *controller, SensorData* sensor_data)
{
    if (controller == NULL) return;
    (void)sensor_data;  // Not needed anymore
    
    // Use standardized command from cmd_controller
    controller->enabled = s_last_cmd.friction_enabled;
    
    // Set turntable target (only feed when feed_enabled)
    float turntable_target = s_last_cmd.feed_enabled ? MOTOR5_CONST_SPEED : 0.0f;
    
    // Set shooter wheel targets
    float shooter1_target = s_last_cmd.friction_enabled ? -SHOOTER_CONST_SPEED : 0.0f;
    float shooter2_target = s_last_cmd.friction_enabled ?  SHOOTER_CONST_SPEED : 0.0f;
    
    // Apply ramping
    controller->ramped_turntable = RampTowards(controller->ramped_turntable, turntable_target, SHOOTER_RAMP_STEP);
    controller->ramped_shooter1 = RampTowards(controller->ramped_shooter1, shooter1_target, SHOOTER_RAMP_STEP);
    controller->ramped_shooter2 = RampTowards(controller->ramped_shooter2, shooter2_target, SHOOTER_RAMP_STEP);
}

void ShooterController_ComputeCurrents(ShooterController *controller, uint32_t current_tick)
{
    if (controller == NULL) return;

    // Compute currents for each shooter motor
    controller->output_currents[0] = ComputeSingleMotorCurrent(&controller->turntable_pid, controller->ramped_turntable, &controller->turntable_feedback, current_tick);
    controller->output_currents[1] = ComputeSingleMotorCurrent(&controller->shooter1_pid, controller->ramped_shooter1, &controller->shooter1_feedback, current_tick);
    controller->output_currents[2] = 0;  // Not used
    controller->output_currents[3] = ComputeSingleMotorCurrent(&controller->shooter2_pid, controller->ramped_shooter2, &controller->shooter2_feedback, current_tick);

    // Send motor currents (module layer handles CAN)
    if (s_feed_motor_id != 0xFF) {
        (void)MotorService_CommandConfigured(s_feed_motor_id,
                                             controller->ramped_turntable,
                                             controller->output_currents[0]);
    }
    if (s_friction1_motor_id != 0xFF) {
        (void)MotorService_CommandConfigured(s_friction1_motor_id,
                                             controller->ramped_shooter1,
                                             controller->output_currents[1]);
    }
    if (s_friction2_motor_id != 0xFF) {
        (void)MotorService_CommandConfigured(s_friction2_motor_id,
                                             controller->ramped_shooter2,
                                             controller->output_currents[3]);
    }
}

void ShooterController_SetTurntableSpeed(ShooterController *controller, float speed)
{ if (controller == NULL) return; controller->turntable_target = speed; }

void ShooterController_SetShooterSpeeds(ShooterController *controller, float shooter1_speed, float shooter2_speed)
{ if (controller == NULL) return; controller->shooter1_target = shooter1_speed; controller->shooter2_target = shooter2_speed; }

void ShooterController_Stop(ShooterController *controller)
{
    if (controller == NULL) return;
    controller->enabled = false;
    controller->turntable_target = 0.0f;
    controller->shooter1_target = 0.0f;
    controller->shooter2_target = 0.0f;
    controller->turntable_pid.integral = 0.0f;
    controller->shooter1_pid.integral = 0.0f;
    controller->shooter2_pid.integral = 0.0f;
}

const int16_t* ShooterController_GetOutputCurrents(const ShooterController *controller)
{ if (controller == NULL) return NULL; return controller->output_currents; }

bool ShooterController_IsRunning(const ShooterController *controller)
{
    if (controller == NULL) return false;
    return controller->enabled || 
           controller->ramped_turntable != 0 || 
           controller->ramped_shooter1 != 0 || 
           controller->ramped_shooter2 != 0;
}

void ShooterController_UpdateMotorFeedback(ShooterController *controller, uint8_t motor_id, uint16_t angle, int16_t speed, int16_t current, uint8_t temp, uint32_t current_tick)
{
    if (controller == NULL) return;

    Motor_Feedback *feedback = NULL;

    // Dynamically match motor_id to feedback structure
    if (motor_id == s_feed_motor_id) {
        feedback = &controller->turntable_feedback;
    }
    else if (motor_id == s_friction1_motor_id) {
        feedback = &controller->shooter1_feedback;
    }
    else if (motor_id == s_friction2_motor_id) {
        feedback = &controller->shooter2_feedback;
    }
    else {
        return;  // Not a shooter motor
    }

    if (feedback != NULL) {
        feedback->angle = angle;
        feedback->speed = speed;
        feedback->current = current;
        feedback->temp = temp;
        feedback->last_update_time = current_tick;
    }
}

// Subscription callbacks
static void on_shoot_cmd(const MsgEvent *ev, void *user) {
    (void)user;
    if (ev->size == sizeof(ShootCmd)) {
        memcpy(&s_last_cmd, ev->data, sizeof(ShootCmd));
        // Update controller and compute currents when command arrives
        ShooterController_Update(&s_ctrl, &s_last_sensor);
        ShooterController_ComputeCurrents(&s_ctrl, BspTime_NowMs());
    }
}

static void on_imu_update(const MsgEvent *ev, void *user) {
    (void)user;
    if (ev->size == sizeof(SensorData)) {
        memcpy(&s_last_sensor, ev->data, sizeof(SensorData));
    }
}

static void on_motor_feedback(const MsgEvent *ev, void *user) {
    (void)user;
    if (ev->size == sizeof(MotorFeedbackEvent)) {
        const MotorFeedbackEvent *m = (const MotorFeedbackEvent *)ev->data;

        // Check if this motor is a shooter motor
        if (m->id == s_feed_motor_id || m->id == s_friction1_motor_id || m->id == s_friction2_motor_id) {
            ShooterController_UpdateMotorFeedback(&s_ctrl, m->id, m->angle, m->speed, m->current, m->temp, m->tick_ms);
        }
    }
}

void ShooterApp_Init(void) {
    memset(&s_last_cmd, 0, sizeof(s_last_cmd));
    memset(&s_last_sensor, 0, sizeof(s_last_sensor));

    // Initialize shooter controller (module layer handles config)
    ShooterController_Init(&s_ctrl);

    (void)MsgCenter_Subscribe(TOPIC_SHOOT_CMD, on_shoot_cmd, NULL);
    (void)MsgCenter_Subscribe(TOPIC_IMU_UPDATE, on_imu_update, NULL);
    (void)MsgCenter_Subscribe(TOPIC_MOTOR_FEEDBACK, on_motor_feedback, NULL);
}

ShooterController* ShooterApp_GetController(void) {
    return &s_ctrl;
}
