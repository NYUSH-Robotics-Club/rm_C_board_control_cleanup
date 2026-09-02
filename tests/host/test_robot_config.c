/* Check both selectable robot configurations for IDs, channels, and safe modes. */
#include <assert.h>
#include <stdbool.h>
#include "robot_config.h"

int main(void)
{
    const RobotConfig_t *robot = RobotConfig_Get();
    assert(robot && robot->motor_configs && robot->total_motor_count <= 16U);
    bool logical_id_used[16] = {false};
    for (uint8_t index = 0U; index < robot->total_motor_count; ++index) {
        const MotorConfig_t *motor = &robot->motor_configs[index];
        assert(motor->motor_id < 16U && !logical_id_used[motor->motor_id]);
        logical_id_used[motor->motor_id] = true;
        assert(motor->vendor == MOTOR_VENDOR_DJI);
        assert(motor->control_mode == MOTOR_CONTROL_APPLICATION);
        assert(motor->can_channel < CAN_CHANNEL_COUNT);
        assert(motor->direction == 1 || motor->direction == -1);
        for (uint8_t other = 0U; other < index; ++other) {
            const MotorConfig_t *previous = &robot->motor_configs[other];
            assert(previous->can_channel != motor->can_channel ||
                   previous->can_rx_id != motor->can_rx_id);
        }
        if (motor->role == MOTOR_ROLE_GIMBAL_YAW ||
            motor->role == MOTOR_ROLE_GIMBAL_PITCH) {
            assert(motor->limits.gm6020.initial_angle < 0.0f);
        }
    }
    return 0;
}
