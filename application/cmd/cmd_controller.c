/*
 * Receives decoded input messages, asks CommandRouter for one command set, and
 * publishes it. Mode policy stays in command_router.c; no motor is driven here.
 */
#include "cmd_controller.h"
#include "command_router.h"
#include "logger.h"
#include "message_center.h"
#include <string.h>

static CommandRouterInput s_input;
static CommandRouterOutput s_output;
static CommandRouter s_router;
static bool s_initialized = false;
static bool s_remote_updated;
static bool s_remote_seen;
static uint32_t s_last_remote_ms;

#define REMOTE_LOSS_TIMEOUT_MS (200U)

static void on_rc_update(const MsgEvent *event, void *user_data) {
    (void)user_data;
    if (event->size == sizeof(s_input.remote)) {
        memcpy(&s_input.remote, event->data, sizeof(s_input.remote));
        s_remote_updated = true;
    }
}

static void on_imu_update(const MsgEvent *event, void *user_data) {
    (void)user_data;
    if (event->size == sizeof(s_input.sensor)) {
        memcpy(&s_input.sensor, event->data, sizeof(s_input.sensor));
    }
}

static void on_vision_update(const MsgEvent *event, void *user_data) {
    (void)user_data;
    if (event->size == sizeof(s_input.vision)) {
        memcpy(&s_input.vision, event->data, sizeof(s_input.vision));
        s_input.vision_updated = true;
    }
}

void CmdController_Init(void) {
    if (s_initialized) {
        return;
    }

    memset(&s_input, 0, sizeof(s_input));
    memset(&s_output, 0, sizeof(s_output));
    s_remote_updated = false;
    s_remote_seen = false;
    s_last_remote_ms = 0U;
    CommandRouter_Init(&s_router);

    (void)MsgCenter_Subscribe(TOPIC_RC_UPDATE, on_rc_update, NULL);
    (void)MsgCenter_Subscribe(TOPIC_IMU_UPDATE, on_imu_update, NULL);
    (void)MsgCenter_Subscribe(TOPIC_VISION_TARGET, on_vision_update, NULL);
    s_initialized = true;
}

void CmdController_Task(uint32_t current_tick) {
    if (!s_initialized) {
        return;
    }

    if (s_remote_updated) {
        s_remote_updated = false;
        s_remote_seen = true;
        s_last_remote_ms = current_tick;
    }
    s_input.remote_online = s_remote_seen &&
        (uint32_t)(current_tick - s_last_remote_ms) <= REMOTE_LOSS_TIMEOUT_MS;

    RobotStatus route_status = CommandRouter_Route(&s_router,
                                                    &s_input,
                                                    current_tick,
                                                    &s_output);
    if (route_status != ROBOT_STATUS_OK &&
        route_status != ROBOT_STATUS_NOT_READY) {
        return;
    }
    s_input.vision_updated = false;

    float yaw_error_deg =
        s_output.spin_hold_yaw_deg - s_input.sensor.yaw_total_angle;
    LOG_CSV(LOG_TAG_CMD, "%u,%u,%u,%.2f,%.2f,%.2f,%.2f,%.3f,%.3f,%.3f,%.3f",
            (unsigned int)(s_output.spin_mode ? 1U : 0U),
            (unsigned int)((uint8_t)s_input.remote.rc.s[0]),
            (unsigned int)((uint8_t)s_input.remote.rc.s[1]),
            s_input.sensor.c_yaw,
            s_input.sensor.yaw_total_angle,
            s_output.spin_hold_yaw_deg,
            yaw_error_deg,
            s_output.gimbal.yaw_rate,
            s_output.chassis.vx,
            s_output.chassis.vy,
            s_output.chassis.wz);

    (void)MsgCenter_Publish(TOPIC_CHASSIS_CMD,
                            &s_output.chassis,
                            sizeof(s_output.chassis));
    (void)MsgCenter_Publish(TOPIC_SHOOT_CMD,
                            &s_output.shooter,
                            sizeof(s_output.shooter));
    (void)MsgCenter_Publish(TOPIC_GIMBAL_CMD,
                            &s_output.gimbal,
                            sizeof(s_output.gimbal));
}
