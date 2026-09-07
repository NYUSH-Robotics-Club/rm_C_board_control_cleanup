/* Check both selectable robot configurations for IDs, channels, and safe modes. */
#include <assert.h>
#include <stdbool.h>
#include "robot_config.h"
#include "dji_motor_protocol.h"

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
        assert(DjiMotor_CommandLimit(motor) > 0);
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
#if defined(ROBOT_TYPE_infantry_standard)
    const MotorConfig_t *yaw = NULL;
    const MotorConfig_t *pitch = NULL;
    for (uint8_t i = 0; i < robot->total_motor_count; ++i) {
        const MotorConfig_t *m = &robot->motor_configs[i];
        if (m->role == MOTOR_ROLE_GIMBAL_YAW) yaw = m;
        if (m->role == MOTOR_ROLE_GIMBAL_PITCH) pitch = m;
    }
    assert(yaw && pitch);
    assert(yaw->can_tx_id == 0x2FE && yaw->can_rx_id == 0x209 && yaw->tx_slot == 0);
    assert(DjiMotor_CommandLimit(yaw) == 4096 && yaw->pid_inner.output_max == 4096);
    assert(yaw->pid_outer.output_max <= 60 && yaw->pid_inner.ki == 0 && yaw->pid_inner.kd == 0);
    assert(pitch->can_tx_id == 0x1FF && pitch->tx_slot == 3);
    assert(pitch->protocol.dji.gm6020_mode == GM6020_COMMAND_VOLTAGE);
    assert(DjiMotor_CommandLimit(pitch) == 25000);
#endif
    return 0;
}
