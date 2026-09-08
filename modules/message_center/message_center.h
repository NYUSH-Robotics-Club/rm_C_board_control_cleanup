/*
 * 声明模块间使用的消息中心。
 * 它不是 RTOS 调度器；回调由唯一的控制任务集中派发。
 */
#ifndef MESSAGE_CENTER_H
#define MESSAGE_CENTER_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Topics aligned with plan; extend as needed
typedef enum {
    TOPIC_RC_UPDATE = 0,
    TOPIC_IMU_UPDATE,
    TOPIC_CAN_RX,
    TOPIC_MOTOR_FEEDBACK,
    TOPIC_GM6020_FEEDBACK,
    TOPIC_CHASSIS_CMD,
    TOPIC_SHOOT_CMD,
    TOPIC_GIMBAL_CMD,
    TOPIC_VISION_DATA,     // Vision data from upper computer
    TOPIC_VISION_TARGET,   // Transport-neutral VisionTargetMessage
    TOPIC_ROBOT_STATE,     // Transport-neutral VisionRobotStateMessage
    TOPIC_NUM_TOPICS
} MsgTopic;

#ifndef MC_MAX_PAYLOAD
#define MC_MAX_PAYLOAD 128
#endif

#ifdef __cplusplus
#define MC_PAYLOAD_ALIGN alignas(4)
#else
#define MC_PAYLOAD_ALIGN _Alignas(4)
#endif

typedef struct {
    uint16_t topic;     // MsgTopic
    uint16_t size;      // bytes used in data
    MC_PAYLOAD_ALIGN uint8_t data[MC_MAX_PAYLOAD];
} MsgEvent;

#undef MC_PAYLOAD_ALIGN

typedef void (*MsgCallback)(const MsgEvent *ev, void *user_data);
typedef void (*MsgDispatchHook)(void *user_data);

// Initialize message center with user-provided ring buffer
void MsgCenter_Init(MsgEvent *buffer, size_t length);

// Publish event (ISR-safe). Returns 0 on success.
int MsgCenter_Publish(MsgTopic topic, const void *data, size_t size);

// Subscribe to a topic; returns 0 on success.
int MsgCenter_Subscribe(MsgTopic topic, MsgCallback cb, void *user_data);

/* A bounded batch guarantees the control loop and transport hooks get CPU time. */
#define MC_DISPATCH_BUDGET 64U
typedef struct {
    uint32_t dispatches;
    uint32_t events;
    uint32_t budget_hits;
    uint32_t overwritten;
} MsgCenterDiagnostics;
const MsgCenterDiagnostics *MsgCenter_GetDiagnostics(void);
// Dispatch at most MC_DISPATCH_BUDGET events; only the designated task may call this.
void MsgCenter_Dispatch(void);

/*
 * Register a generic end-of-dispatch hook. This is intended for composition
 * concerns such as flushing buffered transports. The message center does not
 * know which service owns the hook.
 */
void MsgCenter_SetAfterDispatchHook(MsgDispatchHook hook, void *user_data);
int MsgCenter_AddAfterDispatchHook(MsgDispatchHook hook, void *user_data);

#ifdef __cplusplus
}
#endif

#endif // MESSAGE_CENTER_H
