/* Check queue order, full-queue behavior, subscriptions, and dispatch hooks. */
#include <assert.h>
#include <stdint.h>
#include "message_center.h"

static int s_received = 0;
static int s_hook_calls = 0;

static void on_message(const MsgEvent *event, void *user_data)
{
    (void)user_data;
    assert(event->topic == TOPIC_ROBOT_STATE);
    assert(event->size == sizeof(uint32_t));
    s_received = (int)*(const uint32_t *)event->data;
}

static void after_dispatch(void *user_data)
{
    (void)user_data;
    ++s_hook_calls;
}

int main(void)
{
    MsgEvent queue[8];
    uint32_t value = 42U;
    MsgCenter_Init(queue, 8U);
    assert(MsgCenter_Subscribe(TOPIC_ROBOT_STATE, on_message, 0) == 0);
    assert(MsgCenter_AddAfterDispatchHook(after_dispatch, 0) == 0);
    assert(MsgCenter_Publish(TOPIC_ROBOT_STATE, &value, sizeof(value)) == 0);
    MsgCenter_Dispatch();
    assert(s_received == 42);
    assert(s_hook_calls == 1);
    return 0;
}
