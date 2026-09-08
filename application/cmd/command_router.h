/*
 * Converts decoded operator, sensor, and vision inputs into the three robot
 * command messages. It owns mode priority but does not publish or drive motors.
 */
#ifndef COMMAND_ROUTER_H
#define COMMAND_ROUTER_H

#include <stdbool.h>
#include <stdint.h>
#include "control_messages.h"
#include "remote_messages.h"
#include "robot_status.h"
#include "sensor_messages.h"
#include "vision_messages.h"

typedef struct {
    RemoteControlMessage remote;
    SensorData sensor;
    VisionTargetMessage vision;
    bool vision_updated;
    bool remote_online;
} CommandRouterInput;

typedef struct {
    ChassisCmd chassis;
    ShootCmd shooter;
    GimbalCmd gimbal;
    bool spin_mode;
    bool gimbal_follow_mode;
    float spin_hold_yaw_deg;
} CommandRouterOutput;

typedef struct {
    bool spin_mode;
    bool gimbal_follow_mode;
    float spin_hold_yaw_deg;
    float previous_yaw_input;
    uint32_t previous_route_ms;
    bool route_time_valid;
    bool shooter_down_seen; /* Startup or link loss requires a fresh down position. */
    GimbalCmd gimbal_memory;
} CommandRouter;

void CommandRouter_Init(CommandRouter *router);

/*
 * Build one command set. now_ms is used only for vision expiry; output values
 * are normalized except for documented angle fields in GimbalCmd.
 */
RobotStatus CommandRouter_Route(CommandRouter *router,
                                const CommandRouterInput *input,
                                uint32_t now_ms,
                                CommandRouterOutput *output);

#endif
