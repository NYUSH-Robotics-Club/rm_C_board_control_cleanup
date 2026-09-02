/*
 * 管理两路板级 CAN 收发与诊断。
 * 上层只看到标准帧和通道号，不直接访问 HAL 寄存器。
 */
#ifndef BSP_CAN_H
#define BSP_CAN_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    BSP_CAN_CHANNEL_1 = 1,
    BSP_CAN_CHANNEL_2 = 2
} BspCanChannel;

typedef struct {
    uint32_t error;
    uint32_t error_status_register;
    uint32_t transmit_status_register;
    uint32_t receive_fifo0_register;
    uint32_t free_tx_mailboxes;
} BspCanDiagnostics;

typedef struct {
    uint32_t standard_id;
    uint8_t length;
    uint8_t data[8];
    bool is_standard_frame;
    bool is_data_frame;
} BspCanFrame;

/* 使用当前板卡的固定过滤器配置启动通道。 */
bool BspCan_Start(BspCanChannel channel);

/* 从通道的 FIFO0 读取一帧；无数据或硬件错误均返回 false。 */
bool BspCan_Read(BspCanChannel channel, BspCanFrame *frame);

/* 发送 0～8 字节的标准数据帧。 */
bool BspCan_Write(BspCanChannel channel,
                  uint16_t standard_id,
                  const uint8_t *data,
                  uint8_t length);

/* 仅为冻结中断回调的旧句柄提供通道识别。 */
bool BspCan_MatchesNativeHandle(BspCanChannel channel, const void *native_handle);

bool BspCan_ReadDiagnostics(BspCanChannel channel, BspCanDiagnostics *diagnostics);

#endif
