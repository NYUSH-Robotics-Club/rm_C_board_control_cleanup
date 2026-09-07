/*
 * 保存步兵麦轮机器人使用的电机、方向、限幅和控制参数。
 * 这些值会直接影响实车，修改后必须重新编译并上板验证。
 */
#include "infantry_standard.h"

/**
 * @brief Standard Infantry Robot Configuration
 *
 * This configuration matches the current hardcoded values exactly:
 * - 4x M3508 chassis motors (mecanum wheels)
 * - 2x GM6020 gimbal motors (pitch + yaw)
 * - 3x M3508 shooter motors (turntable + 2x friction wheels)
 */

// Motor configuration array
static const MotorConfig_t g_motor_configs_infantry_standard[] = {
    // ========== CHASSIS MOTORS (4x M3508) ==========
    // Front-left chassis motor (ID 0)
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
        .direction = -1, // Mecanum kinematics correction
        .limits.m3508 = {.speed_limit = 10000.0f},
        .pid_outer = {10.0f, 0.0f, 0.1f, 15000.0f, 7500.0f}, // Speed PID
        .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}          // Not used
    },

    // Front-right chassis motor (ID 1)
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
     .pid_outer = {10.0f, 0.0f, 0.1f, 15000.0f, 7500.0f},
     .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // Back-left chassis motor (ID 2)
    {.motor_id = 3,
     .vendor = MOTOR_VENDOR_DJI,
     .type = MOTOR_TYPE_M3508,
     .role = MOTOR_ROLE_CHASSIS_DRIVE,
     .control_mode = MOTOR_CONTROL_APPLICATION,
     .can_channel = CAN_CHANNEL_1,
     .can_rx_id = 0x203,
     .can_tx_id = 0x200,
     .tx_slot = 2,
     .direction = +1,
     .limits.m3508 = {.speed_limit = 10000.0f},
     .pid_outer = {10.0f, 0.0f, 0.1f, 15000.0f, 7500.0f},
     .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // Back-right chassis motor (ID 3)
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
     .pid_outer = {10.0f, 0.0f, 0.1f, 15000.0f, 7500.0f},
     .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // ========== SHOOTER MOTORS (3x M3508) ==========
    // shooter feed motor (ID 3)
    {.motor_id = 3,
     .vendor = MOTOR_VENDOR_DJI,
     .type = MOTOR_TYPE_M3508,
     .role = MOTOR_ROLE_SHOOTER_FEED,
     .control_mode = MOTOR_CONTROL_APPLICATION,
     .can_channel = CAN_CHANNEL_2,
     .can_rx_id = 0x205,
     .can_tx_id = 0x1FF,
     .tx_slot = 0,
     .direction = +1,
     .limits.m3508 = {.speed_limit = 10000.0f},
     .pid_outer =
         {1.0f, 0.0f, 0.0f, 15000.0f,
          7500.0f}, // Shooter feed PID (reduced Kp to 1.2, increased Kd to 0.3
                    // to suppress high-frequency oscillation)
     .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // Friction wheel 1 (ID 1)//left
    {.motor_id = 1,
     .vendor = MOTOR_VENDOR_DJI,
     .type = MOTOR_TYPE_M3508,
     .role = MOTOR_ROLE_SHOOTER_FRICTION,
     .control_mode = MOTOR_CONTROL_APPLICATION,
     .can_channel = CAN_CHANNEL_2,
     .can_rx_id = 0x206,
     .can_tx_id = 0x1FF,
     .tx_slot = 1,
     .direction = +1,
     .limits.m3508 = {.speed_limit = 10000.0f},
     .pid_outer = {5.0f, 0.5f, 0.1f, 15000.0f, 7500.0f},
     .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // Friction wheel 2 (ID 2)//right
    // Note: motor_id 8 != CAN RX mapping (0x208-0x201=7), but avoids conflict
    // with pitch motor_id 7
    {.motor_id = 2,
     .vendor = MOTOR_VENDOR_DJI,
     .type = MOTOR_TYPE_M3508,
     .role = MOTOR_ROLE_SHOOTER_FRICTION,
     .control_mode = MOTOR_CONTROL_APPLICATION,
     .can_channel = CAN_CHANNEL_2,
     .can_rx_id = 0x208,
     .can_tx_id = 0x1FF,
     .tx_slot = 3, // Slot 3 in TX frame
     .direction = +1,
     .limits.m3508 = {.speed_limit = 10000.0f},
     .pid_outer = {5.0f, 0.5f, 0.1f, 15000.0f, 7500.0f},
     .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // ========== GIMBAL MOTORS (2x GM6020) ==========
    // Yaw gimbal motor (ID 6)
    {
        .motor_id = 5,
        .vendor = MOTOR_VENDOR_DJI,
        .type = MOTOR_TYPE_GM6020,
        .role = MOTOR_ROLE_GIMBAL_YAW,
        .control_mode = MOTOR_CONTROL_APPLICATION,
        .can_channel = CAN_CHANNEL_1,
        .can_rx_id = 0x20A, // GM6020: 0x204 + motor_id
        .can_tx_id = 0x2FF, // Motors 5-7 use 0x2FF
        .tx_slot = 1,       // Motor 6 -> slot 1 (motor_id - 5)
        .direction = +1,
        .limits.gm6020 =
            {
                .angle_min = 0.0f,
                .angle_max = 8192.0f,
                .gravity_compensation = 0.0f,
                .initial_angle = -1.0f // Auto-initialize from current position (no startup vibration)
            },
        .pid_outer = {1.5f, 0.03f, 0.0f, 600.0f, 450.0f}, // Yaw angle PID (Kp=1.5, Ki=0.03, Kd=0.0)
        .pid_inner = {52.5f, 0.12f, 1.8f, 30000.0f, 6000.0f} // Yaw speed PID (Kp=52.5, Ki=0.12, Kd=1.8 for smooth damping)
    },

    // Pitch gimbal motor (ID 7)
    {
        .motor_id = 4, // GM6020 hardware motor ID 7 (CAN RX 0x20B = 0x204 + 7)
        .vendor = MOTOR_VENDOR_DJI,
        .type = MOTOR_TYPE_GM6020,
        .role = MOTOR_ROLE_GIMBAL_PITCH,
        .control_mode = MOTOR_CONTROL_APPLICATION,
        .can_channel = CAN_CHANNEL_2,
        .can_rx_id = 0x20B, // GM6020: 0x204 + 7
        .can_tx_id = 0x2FF,
        .tx_slot = 2,    // Motor 7 -> slot 2 (motor_id - 5)
        .direction = -1, // Pitch direction correction
        .limits.gm6020 =
            {
                .angle_min = 1000.0f,
                .angle_max = 4000.0f,
                .gravity_compensation =
                    5000.0f,             // Gravity compensation for pitch
                // Capture the real power-on angle before enabling position hold.
                .initial_angle = -1.0f
            },
        .pid_outer = {28.0f, 0.0f, 0.5f, 30000.0f, 25000.0f}, // Pitch PID (aggressive: high Kp, low Kd for fast tracking)
        .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f} // Not used for pitch
    }};

// Robot configuration structure
const RobotConfig_t g_robot_config_infantry_standard = {
    .name = "Infantry Standard",
    .chassis_type = CHASSIS_TYPE_MECANUM,
    .chassis_motor_count = 4,
    .gimbal_motor_count = 2,
    .shooter_motor_count = 3,
    .motor_configs = g_motor_configs_infantry_standard,
    .total_motor_count = 9,
    .enable_imu_calibration = 1 // Infantry needs IMU calibration
};
