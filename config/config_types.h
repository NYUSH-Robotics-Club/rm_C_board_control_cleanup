/*
 * 定义车型、电机、控制模式和机器人配置的公共数据结构。
 * 这里只描述数据，不包含硬件读写或控制流程。
 */
#ifndef CONFIG_TYPES_H
#define CONFIG_TYPES_H

#include <stdint.h>

/** Chassis kinematics family selected by a robot configuration. */
typedef enum {
    CHASSIS_TYPE_MECANUM = 0,
    CHASSIS_TYPE_SWERVE,
    CHASSIS_TYPE_OMNI
} ChassisType_e;

/**
 * @brief CAN channel enumeration
 */
typedef enum {
    CAN_CHANNEL_1 = 0,
    CAN_CHANNEL_2 = 1,
    CAN_CHANNEL_COUNT
} CAN_Channel_t;

/**
 * @brief Motor type enumeration
 */
typedef enum {
    MOTOR_TYPE_M3508,    // M3508 brushless motor with C620 ESC
    MOTOR_TYPE_GM6020,   // GM6020 brushless gimbal motor
    MOTOR_TYPE_M2006,    // M2006 motor (for future use)
    MOTOR_TYPE_VENDOR_DEFINED = 255
} MotorType_e;

/** Motor protocol/vendor. Zero intentionally preserves all existing configs. */
typedef enum {
    MOTOR_VENDOR_DJI = 0,
    MOTOR_VENDOR_DM,
    MOTOR_VENDOR_BENMO,
    MOTOR_VENDOR_LK
} MotorVendor_e;

/* 本末 BM1505B 的两种运行指令格式。 */
typedef enum {
    BENMO_CONTROL_RAW_CURRENT = 0x01,
    BENMO_CONTROL_SPEED_RPM_X10 = 0x02
} BenmoControlMode_e;

/**
 * Per-motor control switch. APPLICATION is the compatibility mode used by the
 * existing controllers. Other modes are executed by MotorService when the
 * application supplies a configured setpoint.
 */
typedef enum {
    MOTOR_CONTROL_APPLICATION = 0,
    MOTOR_CONTROL_DISABLED,
    MOTOR_CONTROL_OPEN_LOOP_CURRENT,
    MOTOR_CONTROL_SPEED,
    MOTOR_CONTROL_POSITION,
    MOTOR_CONTROL_POSITION_SPEED_CASCADE
} MotorControlMode_e;

/**
 * @brief Motor role enumeration
 * Defines the functional role of each motor in the robot system
 */
typedef enum {
    MOTOR_ROLE_CHASSIS_DRIVE,      // Chassis drive motor (mecanum or swerve)
    
    MOTOR_ROLE_CHASSIS_STEER,      // Swerve drive steering motor
    MOTOR_ROLE_GIMBAL_YAW,         // Gimbal yaw axis
    MOTOR_ROLE_GIMBAL_PITCH,       // Gimbal pitch axis
    MOTOR_ROLE_SHOOTER_FEED,       // Shooter feed/turntable motor
    MOTOR_ROLE_SHOOTER_FRICTION    // Shooter friction wheel motor
} MotorRole_e;

/**
 * @brief PID parameters structure
 */
typedef struct {
    float kp;              // Proportional gain
    float ki;              // Integral gain
    float kd;              // Derivative gain
    float output_max;      // Maximum output value
    float integral_max;    // Maximum integral value (anti-windup)
} PIDParams_t;

/**
 * @brief Motor configuration structure
 * Contains all parameters needed to configure a single motor
 */
typedef struct {
    // Basic identification
    uint8_t motor_id;              // Logical motor ID (0-15)
    MotorVendor_e vendor;          // CAN protocol/vendor adapter
    MotorType_e type;              // Motor type
    MotorRole_e role;              // Functional role
    MotorControlMode_e control_mode; // Runtime command interpretation

    // CAN communication parameters
    CAN_Channel_t can_channel;     // CAN bus channel (CAN_CHANNEL_1 or CAN_CHANNEL_2)
    uint16_t can_rx_id;            // CAN ID for receiving feedback
    uint16_t can_tx_id;            // CAN ID for sending commands (0x200, 0x1FF, or 0x2FF)
    uint8_t tx_slot;               // Slot position in TX frame (0-3)

    // Motor-specific parameters
    int8_t direction;              // Motor direction: +1 or -1

    // Type-specific limits (union to save memory)
    union {
        // GM6020-specific parameters
        struct {
            float angle_min;            // Minimum angle limit (encoder units)
            float angle_max;            // Maximum angle limit (encoder units)
            float gravity_compensation; // Gravity compensation torque (for pitch axis)
            float initial_angle;        // Initial calibration angle (encoder units)
        } gm6020;

        // M3508-specific parameters
        struct {
            float speed_limit;          // Maximum speed (RPM)
        } m3508;
    } limits;

    /*
     * 厂商协议参数。新增型号时只改所选车型配置，不把型号常数写进驱动。
     * DJI 不使用此联合体，原有配置保持全零即可。
     */
    union {
        struct {
            float position_min;
            float position_max;
            float velocity_min;
            float velocity_max;
            float torque_min;
            float torque_max;
        } dm;
        struct {
            int16_t command_limit;
            uint32_t encoder_counts_per_rev;
        } lk;
        struct {
            uint8_t motor_address;
            BenmoControlMode_e command_mode;
            uint8_t feedback_mode;
            int16_t command_limit;
        } benmo;
    } protocol;

    // PID control parameters
    PIDParams_t pid_outer;         // Outer loop PID (angle/position control)
    PIDParams_t pid_inner;         // Inner loop PID (speed control)
} MotorConfig_t;

/**
 * @brief Robot configuration structure
 * Top-level configuration for a complete robot
 */
typedef struct {
    const char *name;                    // Robot configuration name
    ChassisType_e chassis_type;          // Selected chassis strategy
    uint8_t chassis_motor_count;         // Number of chassis motors
    uint8_t gimbal_motor_count;          // Number of gimbal motors
    uint8_t shooter_motor_count;         // Number of shooter motors
    const MotorConfig_t *motor_configs;  // Pointer to motor configuration array
    uint8_t total_motor_count;           // Total number of motors
    uint8_t enable_imu_calibration;      // Enable IMU calibration at startup (1 = enabled, 0 = disabled)
} RobotConfig_t;

#endif // CONFIG_TYPES_H
