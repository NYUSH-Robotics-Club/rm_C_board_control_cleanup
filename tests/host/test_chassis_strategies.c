/* Check implemented chassis math and safe behavior of unfinished strategies. */
#include <assert.h>
#include "chassis_strategy.h"

int main(void)
{
    const ChassisStrategy *mecanum = MecanumChassisStrategy_Get();
    ChassisKinematicsInput input = {
        .vx = 1.0f,
        .vy = 0.0f,
        .wz = 0.0f,
        .max_drive_speed = 100.0f
    };
    ChassisKinematicsOutput output;
    assert(mecanum->implemented);
    assert(mecanum->compute(&input, &output) == ROBOT_STATUS_OK);
    assert(output.drive_count == 4U);
    for (unsigned int i = 0; i < 4U; ++i) {
        assert(output.drive_speed[i] == 50.0f);
    }

    const ChassisStrategy *swerve = SwerveChassisStrategy_Get();
    assert(swerve->execution == CHASSIS_EXECUTION_LEGACY_CONTROLLER);
    assert(swerve->compute(&input, &output) == ROBOT_STATUS_UNSUPPORTED);

    const ChassisStrategy *omni = OmniChassisStrategy_Get();
    assert(!omni->implemented);
    assert(omni->compute(&input, &output) == ROBOT_STATUS_UNSUPPORTED);
    return 0;
}
