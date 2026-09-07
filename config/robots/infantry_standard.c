/*
 * 保存步兵麦轮机器人使用的电机、方向、限幅和控制参数。
 * 这些值会直接影响实车，修改后必须重新编译并上板验证。
 */
#include "infantry_standard.h"

/**
 * @brief Standard Infantry Robot Configuration
 *
 * - 4x M3508 chassis motors (mecanum wheels)
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
    // 左前轮：CAN1 硬件 ID 1，软件编号 1。
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
     .pid_outer = {10.0f, 0.0f, 0.1f, 15000.0f, 7500.0f},
     .pid_inner = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f}},

    // 左后轮：CAN1 硬件 ID 3，软件编号 3。
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

    // 右后轮：CAN1 硬件 ID 4，软件编号 4。
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
     .pid_outer = {5.0f, 0.5f, 0.1f, 15000.0f, 7500.0f},
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
     .pid_outer = {5.0f, 0.5f, 0.1f, 15000.0f, 7500.0f},
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
        .can_tx_id = 0x2FE, // GM6020 ID5 电流指令；电机内部电流环须开启。
        .tx_slot = 0,
        .direction = +1,
        .limits.gm6020 =
            {
                .angle_min = 0.0f,
                .angle_max = 8192.0f,
                .gravity_compensation = 0.0f,
                .initial_angle = -1.0f // Auto-initialize from current position (no startup vibration)
            },
        .protocol.dji = {GM6020_COMMAND_CURRENT, 4096}, // 初始上限0.75A，非协议最大3A。
        // 首次电流模式调试：位置环输出RPM，速度环输出原始电流刻度。
        // 暂不积累积分或使用微分；这些初始参数仍需按实际负载调参。
        .pid_outer = {1.5f, 0.0f, 0.0f, 60.0f, 0.0f},
        .pid_inner = {64.0f, 0.0f, 0.0f, 4096.0f, 0.0f}
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
