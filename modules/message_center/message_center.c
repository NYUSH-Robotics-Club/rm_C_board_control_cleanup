/*
 * 实现固定内存的消息排队、订阅和派发。
 * 队列满时覆盖最旧消息；回调由派发者统一执行。
 */
#include "message_center.h"
#include "bsp_critical.h"
#include <string.h>

#define MC_MAX_SUBS_PER_TOPIC 8
#define MC_MAX_DISPATCH_HOOKS 4

typedef struct {
    MsgCallback cb;
    void *user;
} Subscriber;

static MsgEvent *mc_queue = NULL;
static size_t mc_len = 0;
static volatile size_t mc_head = 0; // write index (next free)
static volatile size_t mc_tail = 0; // read index (oldest)

static Subscriber mc_subs[TOPIC_NUM_TOPICS][MC_MAX_SUBS_PER_TOPIC];
static uint8_t mc_inited = 0;
static MsgDispatchHook mc_after_dispatch_hooks[MC_MAX_DISPATCH_HOOKS];
static void *mc_after_dispatch_users[MC_MAX_DISPATCH_HOOKS];
static MsgCenterDiagnostics mc_diagnostics;

#define MC_CS_ENTER() BspCriticalState _critical_state = BspCritical_Enter()
#define MC_CS_EXIT() BspCritical_Exit(_critical_state)

void MsgCenter_Init(MsgEvent *buffer, size_t length) {
    mc_queue = buffer;
    mc_len = (buffer && length) ? length : 0;
    mc_head = 0;
    mc_tail = 0;
    memset(&mc_diagnostics, 0, sizeof(mc_diagnostics));
    for (size_t t = 0; t < (size_t)TOPIC_NUM_TOPICS; ++t) {
        for (size_t i = 0; i < MC_MAX_SUBS_PER_TOPIC; ++i) {
            mc_subs[t][i].cb = NULL;
            mc_subs[t][i].user = NULL;
        }
    }
    mc_inited = (mc_len > 0);
    for (size_t i = 0; i < MC_MAX_DISPATCH_HOOKS; ++i) {
        mc_after_dispatch_hooks[i] = NULL;
        mc_after_dispatch_users[i] = NULL;
    }
}

static int mc_is_full(void) {
    size_t next = (mc_head + 1U) % mc_len;
    return (next == mc_tail);
}

static int mc_is_empty(void) {
    return (mc_head == mc_tail);
}

int MsgCenter_Publish(MsgTopic topic, const void *data, size_t size) {
    if (!mc_inited || !mc_queue || mc_len == 0) {
        return -1;
    }
    if ((size > MC_MAX_PAYLOAD) || (topic < 0) || (topic >= TOPIC_NUM_TOPICS)) {
        return -2;
    }

    MC_CS_ENTER();

    // Drop oldest if full (overwrite policy: drop oldest)
    if (mc_is_full()) {
        mc_tail = (mc_tail + 1U) % mc_len;
        mc_diagnostics.overwritten++;
    }

    MsgEvent *ev = &mc_queue[mc_head];
    ev->topic = (uint16_t)topic;
    ev->size = (uint16_t)size;
    if (data && size) {
        memcpy(ev->data, data, size);
    }

    mc_head = (mc_head + 1U) % mc_len;

    MC_CS_EXIT();
    return 0;
}

int MsgCenter_Subscribe(MsgTopic topic, MsgCallback cb, void *user_data) {
    if (!mc_inited || (topic < 0) || (topic >= TOPIC_NUM_TOPICS) || !cb) {
        return -1;
    }
    for (size_t i = 0; i < MC_MAX_SUBS_PER_TOPIC; ++i) {
        if (mc_subs[topic][i].cb == NULL) {
            mc_subs[topic][i].cb = cb;
            mc_subs[topic][i].user = user_data;
            return 0;
        }
    }
    return -2;
}

void MsgCenter_Dispatch(void) {
    if (!mc_inited || !mc_queue || mc_len == 0) {
        return;
    }

    /* CAN interrupts can refill forever. Always return after a bounded batch,
     * including when callbacks themselves publish more messages. */
    mc_diagnostics.dispatches++;
    size_t processed = 0U;
    for (; processed < MC_DISPATCH_BUDGET; ++processed) {
        MC_CS_ENTER();
        if (mc_is_empty()) {
            MC_CS_EXIT();
            break;
        }
        MsgEvent ev = mc_queue[mc_tail];
        mc_tail = (mc_tail + 1U) % mc_len;
        MC_CS_EXIT();
        mc_diagnostics.events++;

        MsgTopic t = (MsgTopic)ev.topic;
        if (t < 0 || t >= TOPIC_NUM_TOPICS) {
            continue;
        }
        for (size_t i = 0; i < MC_MAX_SUBS_PER_TOPIC; ++i) {
            if (mc_subs[t][i].cb) {
                mc_subs[t][i].cb(&ev, mc_subs[t][i].user);
            }
        }
    }
    if (processed == MC_DISPATCH_BUDGET) mc_diagnostics.budget_hits++;
    for (size_t i = 0; i < MC_MAX_DISPATCH_HOOKS; ++i) {
        if (mc_after_dispatch_hooks[i]) {
            mc_after_dispatch_hooks[i](mc_after_dispatch_users[i]);
        }
    }
}

const MsgCenterDiagnostics *MsgCenter_GetDiagnostics(void) { return &mc_diagnostics; }

void MsgCenter_SetAfterDispatchHook(MsgDispatchHook hook, void *user_data) {
    MC_CS_ENTER();
    for (size_t i = 0; i < MC_MAX_DISPATCH_HOOKS; ++i) {
        mc_after_dispatch_hooks[i] = NULL;
        mc_after_dispatch_users[i] = NULL;
    }
    mc_after_dispatch_hooks[0] = hook;
    mc_after_dispatch_users[0] = user_data;
    MC_CS_EXIT();
}

int MsgCenter_AddAfterDispatchHook(MsgDispatchHook hook, void *user_data) {
    if (!hook) {
        return -1;
    }
    MC_CS_ENTER();
    for (size_t i = 0; i < MC_MAX_DISPATCH_HOOKS; ++i) {
        if (mc_after_dispatch_hooks[i] == hook) {
            mc_after_dispatch_users[i] = user_data;
            MC_CS_EXIT();
            return 0;
        }
    }
    for (size_t i = 0; i < MC_MAX_DISPATCH_HOOKS; ++i) {
        if (!mc_after_dispatch_hooks[i]) {
            mc_after_dispatch_hooks[i] = hook;
            mc_after_dispatch_users[i] = user_data;
            MC_CS_EXIT();
            return 0;
        }
    }
    MC_CS_EXIT();
    return -2;
}
