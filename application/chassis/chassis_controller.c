/*
 * 接收底盘命令和电机反馈，并计算驱动电流。
 * 车型换算由底盘策略完成，CAN 发送由电机服务完成。
 */
#include "chassis_controller.h"
#include "motor_service.h"
#include <string.h>
#include <stdint.h>
#include "message_center.h"
#include "motor_messages.h"
#include "sensor_messages.h"
#include "logger.h"
#include "control_messages.h"
#include "chassis_strategy.h"
#include "bsp_time.h"

#define MOTOR_FEEDBACK_TIMEOUT_MS (100U)

// Static variables for app wrapper
static ChassisCmd s_last_cmd;
static SensorData s_last_sensor;
static ChassisController s_ctrl;

// Chassis motor configuration (dynamically assigned during init)
static uint8_t s_chassis_motor_ids[CHASSIS_MOTOR_COUNT];
static int8_t s_motor_directions[CHASSIS_MOTOR_COUNT];
static uint8_t s_chassis_motor_count = 0;
static const ChassisStrategy *s_chassis_strategy = NULL;

static void ResetPidIntegrals(ChassisController *controller)
{
    for (int i = 0; i < CHASSIS_MOTOR_COUNT; i++) { controller->speed_pids[i].integral = 0.0f; }
}

static int16_t ComputeSingleMotorCurrent(PID_Controller *pid, float target, Motor_Feedback *feedback, uint32_t current_tick)
{
    if (current_tick - feedback->last_update_time > MOTOR_FEEDBACK_TIMEOUT_MS) { return 0; }
    float current_speed = feedback->speed;
    return (int16_t)PID_Calculate(pid, target, current_speed);
}

void ChassisController_Init(ChassisController *controller)
{
    if (controller == NULL) return;
    
    memset(controller, 0, sizeof(ChassisController));
    s_chassis_strategy = ChassisStrategy_GetActive();

    // Find chassis motors by role (module layer handles config)
    s_chassis_motor_count = MotorService_FindByRole(MOTOR_ROLE_CHASSIS_DRIVE,
                                                    s_chassis_motor_ids,
                                                    CHASSIS_MOTOR_COUNT);


    // Get motor directions and initialize PIDs from configuration service
    for (uint8_t i = 0; i < s_chassis_motor_count; i++) {
        const MotorConfig_t *config =
            MotorService_GetConfig(s_chassis_motor_ids[i]);
        if (config) {
            s_motor_directions[i] = config->direction;

            // Initialize PID from config
            PID_Init(&controller->speed_pids[i],
                     config->pid_outer.kp,
                     config->pid_outer.ki,
                     config->pid_outer.kd,
                     config->pid_outer.output_max,
                     config->pid_outer.integral_max);

            controller->target_speeds[i] = 0.0f;
        }
    }
}

void ChassisController_Update(ChassisController *controller, SensorData* sensor_data)
{
    if (controller == NULL) return;
    (void)sensor_data;

    ChassisKinematicsInput input = {
        .vx = s_last_cmd.vx,
        .vy = s_last_cmd.vy,
        .wz = s_last_cmd.wz,
        .max_drive_speed = (float)CHASSIS_DEMO_TARGET_SPEED
    };
    ChassisKinematicsOutput output;
    RobotStatus status = ROBOT_STATUS_NOT_READY;
    if (s_chassis_strategy && s_chassis_strategy->compute) {
        status = s_chassis_strategy->compute(&input, &output);
    }
    if (status == ROBOT_STATUS_OK) {
        uint8_t count = output.drive_count < s_chassis_motor_count
                            ? output.drive_count
                            : s_chassis_motor_count;
        for (uint8_t i = 0; i < count; ++i) {
            controller->target_speeds[i] =
                s_motor_directions[i] * output.drive_speed[i];
        }
    } else {
        for (uint8_t i = 0; i < s_chassis_motor_count; ++i) {
            controller->target_speeds[i] = 0.0f;
        }
    }

    controller->running = s_last_cmd.enabled;
}

void ChassisController_ComputeCurrents(ChassisController *controller, uint32_t current_tick)
{
    if (controller == NULL) return;
    
    // Chassis status logging (5Hz rate limited in main.c)
    LOG_INFO(LOG_TAG_CHA, "enabled=%d", (int)s_last_cmd.enabled);
    for (uint8_t i = 0; i < s_chassis_motor_count; i++) {
        uint32_t dt = BspTime_NowMs() - controller->motor_feedbacks[i].last_update_time;
        LOG_DEBUG(LOG_TAG_CHA, "motor[%d] id=%d tgt=%.1f fb=%.1f dt=%lu out=%d",
                  i,
                  s_chassis_motor_ids[i],
                  controller->target_speeds[i],
                  controller->motor_feedbacks[i].speed,
                  (unsigned long)dt,
                  controller->output_currents[i]);
    }


    // Compute current for each chassis motor
    for (int i = 0; i < s_chassis_motor_count; i++) {
        int16_t motor_current = ComputeSingleMotorCurrent(
            &controller->speed_pids[i],
            controller->target_speeds[i],
            &controller->motor_feedbacks[i],
            current_tick
        );
        controller->output_currents[i] = motor_current;

        // Send motor current (buffered, will be flushed in main loop)
        (void)MotorService_CommandConfigured(s_chassis_motor_ids[i],
                                             controller->target_speeds[i],
                                             motor_current);
    }
}

void ChassisController_SetTargetSpeeds(ChassisController *controller, const float speeds[CHASSIS_MOTOR_COUNT])
{
    if (controller == NULL || speeds == NULL) return;
    for (int i = 0; i < CHASSIS_MOTOR_COUNT; i++) { controller->target_speeds[i] = speeds[i]; }
}

void ChassisController_Stop(ChassisController *controller)
{
    if (controller == NULL) return;
    controller->running = false;
    ResetPidIntegrals(controller);
    for (int i = 0; i < CHASSIS_MOTOR_COUNT; i++) { controller->target_speeds[i] = 0.0f; }
}

const int16_t* ChassisController_GetOutputCurrents(const ChassisController *controller)
{
    if (controller == NULL) return NULL;
    return controller->output_currents;
}

bool ChassisController_IsRunning(const ChassisController *controller)
{
    if (controller == NULL) return false;
    for (int i = 0; i < CHASSIS_MOTOR_COUNT; i++) { if (controller->target_speeds[i] != 0) { return true; } }
    return false;
}

void ChassisController_UpdateMotorFeedback(ChassisController *controller, uint8_t motor_id, uint16_t angle, int16_t speed, int16_t current, uint8_t temp, uint32_t current_tick)
{

    if (controller == NULL || motor_id >= CHASSIS_MOTOR_COUNT) return;
    controller->motor_feedbacks[motor_id].angle = angle;
    controller->motor_feedbacks[motor_id].speed = speed;
    controller->motor_feedbacks[motor_id].current = current;
    controller->motor_feedbacks[motor_id].temp = temp;
    controller->motor_feedbacks[motor_id].last_update_time = current_tick;
}

// Subscription callbacks
static void on_chassis_cmd(const MsgEvent *ev, void *user) {
    (void)user;
    if (ev->size == sizeof(ChassisCmd)) {
        memcpy(&s_last_cmd, ev->data, sizeof(ChassisCmd));
        // Update controller and compute currents when command arrives
        ChassisController_Update(&s_ctrl, &s_last_sensor);
        ChassisController_ComputeCurrents(&s_ctrl, BspTime_NowMs());
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

        // Check if this motor is a chassis motor
        for (uint8_t i = 0; i < s_chassis_motor_count; i++) {
            if (m->id == s_chassis_motor_ids[i]) {
                // Found matching chassis motor, update feedback at index i
                ChassisController_UpdateMotorFeedback(&s_ctrl, i, m->angle, m->speed, m->current, m->temp, m->tick_ms);
                break;
            }
        }
    }
}

void ChassisApp_Init(void) {
    
    memset(&s_last_cmd, 0, sizeof(s_last_cmd));
    memset(&s_last_sensor, 0, sizeof(s_last_sensor));

    // Initialize chassis controller (module layer handles config)
    ChassisController_Init(&s_ctrl);

    (void)MsgCenter_Subscribe(TOPIC_CHASSIS_CMD, on_chassis_cmd, NULL);
    (void)MsgCenter_Subscribe(TOPIC_IMU_UPDATE, on_imu_update, NULL);
    (void)MsgCenter_Subscribe(TOPIC_MOTOR_FEEDBACK, on_motor_feedback, NULL);
}

ChassisController* ChassisApp_GetController(void) {
    return &s_ctrl;
}
