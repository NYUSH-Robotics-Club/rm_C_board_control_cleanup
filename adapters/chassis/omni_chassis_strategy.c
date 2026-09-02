/*
 * 全向轮的安全占位实现。
 * 轮数和安装角未知时明确返回“不支持”，不会生成运动指令。
 */
#include "chassis_strategy.h"

static RobotStatus omni_compute(const ChassisKinematicsInput *input,
                                ChassisKinematicsOutput *output)
{
    (void)input;
    (void)output;
    /* Wheel count, mounting angles, radius, and reduction ratio are unknown. */
    return ROBOT_STATUS_UNSUPPORTED;
}

const ChassisStrategy *OmniChassisStrategy_Get(void)
{
    static const ChassisStrategy strategy = {
        .type = CHASSIS_TYPE_OMNI,
        .name = "omni-placeholder",
        .execution = CHASSIS_EXECUTION_PLACEHOLDER,
        .implemented = false,
        .compute = omni_compute
    };
    return &strategy;
}
