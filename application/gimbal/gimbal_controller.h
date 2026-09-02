/*
 * 声明云台控制和启动对齐接口。
 * 角速度输入为 -1 到 1 的归一化值，返回值是电流命令。
 */
#ifndef GIMBAL_CONTROLLER_H
#define GIMBAL_CONTROLLER_H

#include <stdint.h>
#include <stdbool.h>
#include "control_messages.h"
#include "sensor_messages.h"

#ifdef __cplusplus
extern "C" {
#endif

void last_data(float last_yaw_rate, float last_yaw_target);

/**
 * @brief Pitch control with normalized rate command
 * @param id Motor ID (7=Pitch)
 * @param rate_normalized Normalized pitch rate (-1.0 to 1.0, always responds to joystick)
 * @param sensor_data Sensor data pointer
 * @param disable_yaw_pitch_compensation Disable yaw-pitch coupling compensation (true in auto-aim mode)
 * @return Motor current command
 */

int16_t GimbalController_PitchControl(uint8_t id, float rate_normalized, SensorData* sensor_data, bool disable_yaw_pitch_compensation);

/**
 * @brief Yaw control with compensation (chassis rotation + gyro feedback)
 * @param rate_normalized Normalized yaw rate (-1.0 to 1.0)
 * @param sensor_data Sensor data pointer
 * @param use_imu_feedback Use IMU gyro for speed feedback (true for spin mode, false for encoder)
 * @return Motor current command
 */
int16_t GimbalController_YawControlWithCompensation(float rate_normalized, SensorData* sensor_data, bool use_imu_feedback);

/**
 * @brief Target angle correction for yaw (chassis compensation)
 * @param sensor_data Sensor data pointer
 */
void GimbalController_TargetAngleCorrection(SensorData* sensor_data);

/**
 * @brief Initialize gimbal application (message subscriptions and control)
 */
void GimbalApp_Init(void);

/**
 * @brief Wait for gimbal feedback and hold the power-on yaw/pitch position
 * @note No fixed mechanical target is selected during startup. Each configured
 *       axis starts holding only after its first encoder feedback is available.
 *       Timeout: 5 seconds
 */
void Gimbal_WaitForAlignment(void);

/**
 * @brief Calculate and display gimbal tilt angle compensation
 * @note Calculates yaw-pitch coupling compensation when gimbal is tilted
 *       Updates every 100ms, displays via USB CDC and logs to CSV
 *       Compensation model assumes 30cm gimbal height above ground
 */
void GimbalController_CalculateAndDisplayCompensation(void);


#ifdef __cplusplus
}
#endif

#endif // GIMBAL_CONTROLLER_H
