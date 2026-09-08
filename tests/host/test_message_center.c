/* Check queue order, full-queue behavior, subscriptions, and dispatch hooks. */
#include <assert.h>
#include <stdint.h>
#include "message_center.h"

static int s_received = 0;
static int s_hook_calls = 0;
static unsigned s_refill_calls;

static void refill_queue(const MsgEvent *event, void *user_data)
{
    (void)user_data;
    ++s_refill_calls;
    if (s_refill_calls < 1000U) {
        assert(MsgCenter_Publish(TOPIC_ROBOT_STATE, event->data, event->size) == 0);
    }
}

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

    MsgCenter_Init(queue, 8U);
    s_hook_calls = 0;
    assert(MsgCenter_Subscribe(TOPIC_ROBOT_STATE, refill_queue, 0) == 0);
    assert(MsgCenter_AddAfterDispatchHook(after_dispatch, 0) == 0);
    MsgCenter_Publish(TOPIC_ROBOT_STATE, &value, sizeof(value));
    MsgCenter_Dispatch();
    assert(s_refill_calls == MC_DISPATCH_BUDGET && s_hook_calls == 1);
    assert(MsgCenter_GetDiagnostics()->budget_hits == 1);
    MsgCenter_Dispatch();
    assert(s_refill_calls == 2U * MC_DISPATCH_BUDGET && s_hook_calls == 2);
    return 0;
}
