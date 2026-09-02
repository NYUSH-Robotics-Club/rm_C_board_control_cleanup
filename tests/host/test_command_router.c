/* Check safe RC loss behavior, mode routing, and time-based spin adjustment. */
#include <assert.h>
#include <math.h>
#include <string.h>
#include "command_router.h"

int main(void)
{
    CommandRouter router;
    CommandRouterInput input;
    CommandRouterOutput output;
    memset(&input, 0, sizeof(input));
    CommandRouter_Init(&router);

    assert(CommandRouter_Route(&router, &input, 100U, &output) ==
           ROBOT_STATUS_NOT_READY);
    assert(!output.chassis.enabled && !output.gimbal.enabled &&
           !output.shooter.feed_enabled);

    input.remote_online = true;
    input.remote.rc.s[0] = RC_SW_UP;
    input.remote.rc.s[1] = RC_SW_UP;
    input.remote.rc.ch[0] = -660;
    input.sensor.yaw_total_angle = 10.0f;
    assert(CommandRouter_Route(&router, &input, 1000U, &output) ==
           ROBOT_STATUS_OK);
    assert(output.spin_mode && output.chassis.enabled);
    assert(output.shooter.feed_enabled && output.shooter.friction_enabled);
    const float first_target = output.spin_hold_yaw_deg;

    assert(CommandRouter_Route(&router, &input, 1010U, &output) ==
           ROBOT_STATUS_OK);
    assert(output.spin_hold_yaw_deg > first_target + 1.0f);
    assert(output.spin_hold_yaw_deg < first_target + 1.3f);

    input.remote_online = false;
    assert(CommandRouter_Route(&router, &input, 1300U, &output) ==
           ROBOT_STATUS_NOT_READY);
    assert(!output.chassis.enabled && !output.gimbal.enabled);
    return 0;
}
