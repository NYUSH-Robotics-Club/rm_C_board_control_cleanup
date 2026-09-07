/* Publish the copied nyush-rm-control decoder's output using existing robot messages.
 * UART reception and DBUS decoding are implemented in the vendored source files. */
#include "remote_control.h"
#include "nyush_remote.h"
#include "bsp_rc.h"
#include "bsp_time.h"
#include "bsp_critical.h"
#include "message_center.h"
#include <string.h>
RC_ctrl_t rc_ctrl;
static volatile uint32_t rc_frame_count;
static uint8_t last_sbus_frame[RC_FRAME_LENGTH];
static uint32_t s_daemon_tick_ms;
static void on_frame(const uint8_t *frame, uint16_t length)
{
    NyushRemoteSnapshot decoded;
    if (length != RC_FRAME_LENGTH || !NyushRemote_GetSnapshot(&decoded)) return;
    memcpy(rc_ctrl.rc.ch, decoded.channels, sizeof(rc_ctrl.rc.ch));
    rc_ctrl.rc.s[0] = decoded.switches[0];
    rc_ctrl.rc.s[1] = decoded.switches[1];
    rc_ctrl.mouse.x = decoded.mouse[0];
    rc_ctrl.mouse.y = decoded.mouse[1];
    rc_ctrl.mouse.z = decoded.mouse[2];
    rc_ctrl.mouse.press_l = decoded.buttons[0];
    rc_ctrl.mouse.press_r = decoded.buttons[1];
    rc_ctrl.key.v = decoded.keys;
    memcpy(last_sbus_frame, frame, RC_FRAME_LENGTH);
    rc_frame_count++;
    (void)MsgCenter_Publish(TOPIC_RC_UPDATE, &rc_ctrl, sizeof(rc_ctrl));
}
static void poll_receiver(void *user)
{
    (void)user;
    uint32_t now_ms = BspTime_NowMs();
    if ((uint32_t)(now_ms - s_daemon_tick_ms) >= 10U) {
        s_daemon_tick_ms = now_ms;
        NyushRemote_DaemonTick();
    }
}
void remote_control_init(void)
{
    BspRc_SetFrameCallback(on_frame);
    s_daemon_tick_ms = BspTime_NowMs();
    /* Upstream RobotInit excludes IRQs until instance and daemon pointers exist. */
    BspCriticalState state = BspCritical_Enter();
    NyushRemote_Init();
    BspCritical_Exit(state);
    (void)MsgCenter_AddAfterDispatchHook(poll_receiver, NULL);
}
uint32_t RC_GetFrameCount(void) { return rc_frame_count; }
const RC_ctrl_t *get_remote_control_point(void) { return &rc_ctrl; }
void RC_GetLastFrame(uint8_t out[RC_FRAME_LENGTH])
{
    if (out) memcpy(out, last_sbus_frame, RC_FRAME_LENGTH);
}
