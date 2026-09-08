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
    assert(robot->chassis_type == CHASSIS_TYPE_OMNI && robot->omni);
    assert(robot->omni->configured);
    const uint8_t wheel_ids[4] = {3, 2, 1, 4};
    for (unsigned i = 0; i < 4; ++i) {
        assert(robot->omni->wheels[i].motor_id == wheel_ids[i]);
        const MotorConfig_t *wheel = NULL;
        for (uint8_t j = 0; j < robot->total_motor_count; ++j)
            if (robot->motor_configs[j].motor_id == wheel_ids[i]) wheel = &robot->motor_configs[j];
        assert(wheel && wheel->role == MOTOR_ROLE_CHASSIS_DRIVE);
        assert(wheel->type == MOTOR_TYPE_M3508 && wheel->can_channel == CAN_CHANNEL_1);
        assert(wheel->can_rx_id == 0x200 + wheel_ids[i]);
        assert(wheel->can_tx_id == 0x200 && wheel->tx_slot == wheel_ids[i] - 1);
    }
    const MotorConfig_t *yaw = NULL;
    const MotorConfig_t *pitch = NULL;
    for (uint8_t i = 0; i < robot->total_motor_count; ++i) {
        const MotorConfig_t *m = &robot->motor_configs[i];
        if (m->role == MOTOR_ROLE_GIMBAL_YAW) yaw = m;
        if (m->role == MOTOR_ROLE_GIMBAL_PITCH) pitch = m;
    }
    assert(yaw && pitch);
    assert(yaw->can_tx_id == 0x2FF && yaw->can_rx_id == 0x209 && yaw->tx_slot == 0);
    assert(yaw->protocol.dji.gm6020_mode == GM6020_COMMAND_VOLTAGE);
    assert(DjiMotor_CommandLimit(yaw) == 25000 && yaw->pid_inner.output_max == 25000);
    assert(yaw->pid_outer.kp == 1.5f && yaw->pid_inner.kp == 52.5f);
    assert(pitch->can_tx_id == 0x1FF && pitch->tx_slot == 3);
    assert(pitch->protocol.dji.gm6020_mode == GM6020_COMMAND_VOLTAGE);
    assert(DjiMotor_CommandLimit(pitch) == 25000);
#endif
    return 0;
}
