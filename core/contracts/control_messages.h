/*
 * 定义应用模块之间传递的底盘、云台和发射命令。
 * 这些结构不包含控制器内部状态或硬件句柄。
 */
#ifndef CONTROL_MESSAGES_H
#define CONTROL_MESSAGES_H

#include <stdbool.h>
#include <stdint.h>

/* Application-level contracts. These types contain no controller or HAL state. */
typedef struct {
    float vx;
    float vy;
    float wz;
    bool enabled;
} ChassisCmd;

typedef struct {
    bool friction_enabled;
    bool feed_enabled;
} ShootCmd;

typedef struct {
    bool enabled;
    float pitch_rate;
    float yaw_rate;
    float yaw_rate_memo;
    float yaw_target_memo;
    bool vision_valid;
    float vision_yaw_err_rad;
    float vision_pitch_err_rad;
    uint32_t vision_ts_ms;
} GimbalCmd;

#endif
