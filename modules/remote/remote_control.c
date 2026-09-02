/*
 * 解析 DR16 遥控器的 SBUS 数据并发布遥控更新消息。
 * USART3 和 DMA 由 BSP 管理，本文件不直接访问寄存器。
 */

#include "remote_control.h"
#include "bsp_rc.h"
#include "message_center.h"
#include "logger.h"
#include <string.h>
/*注意这里是先声明函数，后面有函数定义，这在c语言中是合法的*/
static void sbus_to_rc(volatile const uint8_t *sbus_buf, RC_ctrl_t *rc_ctrl);
static void on_sbus_frame(const uint8_t *frame, uint16_t length);

//remote control data
RC_ctrl_t rc_ctrl;

//receive data, 18 bytes one frame, but set 36 bytes
static uint8_t sbus_rx_buf[2][SBUS_RX_BUF_NUM];
// frame counter
static volatile uint32_t rc_frame_count = 0;
// keep a snapshot of last 18-byte SBUS frame for debugging
static uint8_t last_sbus_frame[RC_FRAME_LENGTH];

uint32_t RC_GetFrameCount(void)
{
  return rc_frame_count;
}

void remote_control_init(void)
{
    BspRc_SetFrameCallback(on_sbus_frame);
    RC_init(sbus_rx_buf[0], sbus_rx_buf[1], SBUS_RX_BUF_NUM);
    LOG_INFO(LOG_TAG_RC, "Remote control initialized, waiting for SBUS data on USART3...");
}

void RC_GetLastFrame(uint8_t out[RC_FRAME_LENGTH])
{
    if (!out) return;
    memcpy(out, last_sbus_frame, RC_FRAME_LENGTH);
}

const RC_ctrl_t *get_remote_control_point(void)
{
    return &rc_ctrl;
}

static void on_sbus_frame(const uint8_t *frame, uint16_t length)
{
    static uint32_t callback_count;
    callback_count++;

    if (!frame || length != RC_FRAME_LENGTH) {
        if ((callback_count % 500U) == 1U) {
            LOG_DEBUG(LOG_TAG_RC, "Wrong frame length: %u (expected %u)",
                      (unsigned int)length, (unsigned int)RC_FRAME_LENGTH);
        }
        return;
    }

    sbus_to_rc(frame, &rc_ctrl);
    memcpy(last_sbus_frame, frame, RC_FRAME_LENGTH);
    rc_frame_count++;
    (void)MsgCenter_Publish(TOPIC_RC_UPDATE, &rc_ctrl, sizeof(rc_ctrl));

    /* This callback still runs from the USART interrupt. Keep work short. */
    if (rc_frame_count == 1U) {
        LOG_INFO(LOG_TAG_RC, "First SBUS frame received! RC link active.");
    }
}

static void sbus_to_rc(volatile const uint8_t *sbus_buf, RC_ctrl_t *rc_ctrl)
{
    if (sbus_buf == NULL || rc_ctrl == NULL)
    {
        return;
    }

    rc_ctrl->rc.ch[0] = (sbus_buf[0] | (sbus_buf[1] << 8)) & 0x07ff;
    rc_ctrl->rc.ch[1] = ((sbus_buf[1] >> 3) | (sbus_buf[2] << 5)) & 0x07ff;
    rc_ctrl->rc.ch[2] = ((sbus_buf[2] >> 6) | (sbus_buf[3] << 2) |
                         (sbus_buf[4] << 10)) &0x07ff;
    rc_ctrl->rc.ch[3] = ((sbus_buf[4] >> 1) | (sbus_buf[5] << 7)) & 0x07ff;
    rc_ctrl->rc.s[0] = ((sbus_buf[5] >> 4) & 0x0003);
    rc_ctrl->rc.s[1] = ((sbus_buf[5] >> 4) & 0x000C) >> 2;
    rc_ctrl->mouse.x = sbus_buf[6] | (sbus_buf[7] << 8);
    rc_ctrl->mouse.y = sbus_buf[8] | (sbus_buf[9] << 8);
    rc_ctrl->mouse.z = sbus_buf[10] | (sbus_buf[11] << 8);
    rc_ctrl->mouse.press_l = sbus_buf[12];
    rc_ctrl->mouse.press_r = sbus_buf[13];
    rc_ctrl->key.v = sbus_buf[14] | (sbus_buf[15] << 8);
    rc_ctrl->rc.ch[4] = (sbus_buf[16] | (sbus_buf[17] << 8)) & 0x07ff;

    rc_ctrl->rc.ch[0] -= RC_CH_VALUE_OFFSET;
    rc_ctrl->rc.ch[1] -= RC_CH_VALUE_OFFSET;
    rc_ctrl->rc.ch[2] -= RC_CH_VALUE_OFFSET;
    rc_ctrl->rc.ch[3] -= RC_CH_VALUE_OFFSET;
    rc_ctrl->rc.ch[4] -= RC_CH_VALUE_OFFSET;
}
