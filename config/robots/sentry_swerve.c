/*
 * 保存现有哨兵舵轮机器人的电机和控制参数。
 * 当前是两转向电机的旧实现，不代表通用四模块舵轮。
 */
#include "sentry_swerve.h"

/**
 * Sentry / Swerve Chassis Config
 * - 4x M3508: wheel drive motors
 * - 2x GM6020: wheel rotator / steering motors
 *
 * NOTE: Shooter/gimbal counts are set to 0 in RobotConfig_t.
 */

// ========== MOTOR LIST ==========
static const MotorConfig_t g_motor_configs_sentry_swerve[] = {

    // --------------------------
    // DRIVE (M3508) — CAN1
    // --------------------------
    // Wheel 0 drive (right-omniwheel)
    {
        .motor_id = 0,
        .vendor = MOTOR_VENDOR_DJI,
        .type = MOTOR_TYPE_M3508,
        .role = MOTOR_ROLE_CHASSIS_DRIVE,
        .control_mode = MOTOR_CONTROL_APPLICATION,
        .can_channel = CAN_CHANNEL_1,
        .can_rx_id = 0x201,
        .can_tx_id = 0x200,
        .tx_slot = 0,
        .direction = 1,
        .limits.m3508 = {.speed_limit = 10000.0f},
        .pid_outer = {10.0f, 0.0f, 0.1f, 15000.0f, 7500.0f},
        .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // Wheel 1 drive (front normal wheel)
    {.motor_id = 1, .vendor = MOTOR_VENDOR_DJI, .type = MOTOR_TYPE_M3508, .role = MOTOR_ROLE_CHASSIS_DRIVE, .control_mode = MOTOR_CONTROL_APPLICATION, .can_channel = CAN_CHANNEL_1, .can_rx_id = 0x202, .can_tx_id = 0x200, .tx_slot = 1, .direction = -1, .limits.m3508 = {.speed_limit = 10000.0f}, .pid_outer = {8.0f, 0.0f, 0.1f, 15000.0f, 7500.0f}, .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // Wheel 2 drive (left-omniwheel)
    {.motor_id = 2, .vendor = MOTOR_VENDOR_DJI, .type = MOTOR_TYPE_M3508, .role = MOTOR_ROLE_CHASSIS_DRIVE, .control_mode = MOTOR_CONTROL_APPLICATION, .can_channel = CAN_CHANNEL_1, .can_rx_id = 0x203, .can_tx_id = 0x200, .tx_slot = 2, .direction = 1, .limits.m3508 = {.speed_limit = 10000.0f}, .pid_outer = {10.0f, 0.0f, 0.1f, 15000.0f, 7500.0f}, .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // Wheel 3 drive (back normal wheel)
    {.motor_id = 3, .vendor = MOTOR_VENDOR_DJI, .type = MOTOR_TYPE_M3508, .role = MOTOR_ROLE_CHASSIS_DRIVE, .control_mode = MOTOR_CONTROL_APPLICATION, .can_channel = CAN_CHANNEL_1, .can_rx_id = 0x204, .can_tx_id = 0x200, .tx_slot = 3, .direction = -1, .limits.m3508 = {.speed_limit = 10000.0f}, .pid_outer = {8.0f, 0.0f, 0.005f, 15000.0f, 7500.0f}, .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // --------------------------
    // STEERING (GM6020) — CAN1
    // --------------------------
    //  (GM6020 ID 5 => RX 0x209)
    {
        .motor_id = 5, .vendor = MOTOR_VENDOR_DJI, .type = MOTOR_TYPE_GM6020, .role = MOTOR_ROLE_CHASSIS_STEER, .control_mode = MOTOR_CONTROL_APPLICATION, .can_channel = CAN_CHANNEL_1,
        .can_rx_id = 0x209, // 0x204 + 5
        .can_tx_id = 0x2FF, // GM6020 ID 5-7 use 0x2FF
        .tx_slot = 0,       // For 0x2FF, slot = (motor_id - 5)
        .direction = 1,
        .limits.gm6020 = {.angle_min = 0.0f, .angle_max = 8192.0f, .gravity_compensation = 0.0f, .initial_angle = 1084.0f},
        .pid_outer = {0.7f, 0.045f, 0.018f, 300.0f, 300.0f}, // Yaw angle PID
        .pid_inner = {22.0f, 0.01f, 3.0f, 30000.0f, 4000.0f} // Yaw speed PID
    },

    // Steer motor B (GM6020 ID 6 => RX 0x20A)
    {
        .motor_id = 6, .vendor = MOTOR_VENDOR_DJI, .type = MOTOR_TYPE_GM6020, .role = MOTOR_ROLE_CHASSIS_STEER, .control_mode = MOTOR_CONTROL_APPLICATION, .can_channel = CAN_CHANNEL_1,
        .can_rx_id = 0x20A, // 0x204 + 6
        .can_tx_id = 0x2FF,
        .tx_slot = 1,
        .direction = 1,
        .limits.gm6020 = {.angle_min = 0.0f, .angle_max = 8192.0f, .gravity_compensation = 0.0f, .initial_angle = 2434.0f},
        .pid_outer = {0.7f, 0.045f, 0.018f, 300.0f, 300.0f},
        .pid_inner = {22.0f, 0.01f, 3.0f, 30000.0f, 4000.0f}},

    // --------------------------
    // GIMBAL YAW (GM6020) — CAN1
    // --------------------------
    // Yaw gimbal motor (GM6020 ID 7 => RX 0x20B)
    // DUAL-LOOP CONTROL: angle → speed → current (same as infantry)
    {
        .motor_id = 7,
        .vendor = MOTOR_VENDOR_DJI,
        .type = MOTOR_TYPE_GM6020,
        .role = MOTOR_ROLE_GIMBAL_YAW,
        .control_mode = MOTOR_CONTROL_APPLICATION,
        .can_channel = CAN_CHANNEL_1,
        .can_rx_id = 0x20B, // 0x204 + 7
        .can_tx_id = 0x2FF, // GM6020 ID 5-7 use 0x2FF
        .tx_slot = 2,       // motor_id - 5 = 7 - 5 = 2
        .direction = 1,
        .limits.gm6020 = {
            .angle_min = 0.0f,
            .angle_max = 8192.0f,
            .gravity_compensation = 0.0f,
            .initial_angle = -1.0f // Auto-init: use current angle on first feedback
        },
        .pid_outer = {0.7f, 0.045f, 0.018f, 300.0f, 300.0f},
        .pid_inner = {22.0f, 0.01f, 3.0f, 30000.0f, 4000.0f} // Speed → Current
    }};

// ========== ROBOT CONFIG ==========
const RobotConfig_t g_robot_config_sentry_swerve = {
    .name = "Sentry Swerve Standard",
    .chassis_type = CHASSIS_TYPE_SWERVE,
    .chassis_motor_count = 6, // 4 drive + 2 steer
    .gimbal_motor_count = 1,  // 1 yaw motor
    .shooter_motor_count = 0,
    .motor_configs = g_motor_configs_sentry_swerve,
    .total_motor_count = 7,   // 6 chassis + 1 gimbal
    .enable_imu_calibration = 0 // Sentry does not need IMU calibration
};
