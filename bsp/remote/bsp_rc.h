/*
 * 管理 DR16 遥控器使用的 USART3 双缓冲 DMA。
 * 完整帧通过回调交给 remote_control 解析，本文件不解释 SBUS 字段。
 */
#ifndef BSP_RC_H
#define BSP_RC_H

#include <stdint.h>

typedef void (*BspRcFrameCallback)(const uint8_t *frame, uint16_t length);

/* Bind the module callback before enabling UART/DMA reception. */
void BspRc_SetFrameCallback(BspRcFrameCallback callback);

/* Compatibility name used by the existing remote module. */
void RC_init(uint8_t *rx1_buf, uint8_t *rx2_buf, uint16_t dma_buf_num);

/* Called by the frozen USART3 interrupt path. */
void REMOTE_USART3_IDLE_IRQHandler(void);

#endif
