/*
 * 实现当前 STM32 板卡的 CAN 启动、收发和诊断。
 * 滤波器分区沿用现有固件设置，不在业务模块里散布 HAL 调用。
 */
#include "bsp_can.h"
#include "main.h"

#include <string.h>

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

static CAN_HandleTypeDef *handle_for(BspCanChannel channel)
{
    if (channel == BSP_CAN_CHANNEL_1) {
        return &hcan1;
    }
    if (channel == BSP_CAN_CHANNEL_2) {
        return &hcan2;
    }
    return NULL;
}

bool BspCan_Start(BspCanChannel channel)
{
    CAN_HandleTypeDef *handle = handle_for(channel);
    if (!handle || !handle->Instance) {
        return false;
    }

    CAN_FilterTypeDef filter = {0};
    filter.FilterBank = channel == BSP_CAN_CHANNEL_1 ? 0U : 14U;
    filter.SlaveStartFilterBank = channel == BSP_CAN_CHANNEL_1 ? 14U : 0U;
    filter.FilterActivation = ENABLE;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterFIFOAssignment = CAN_FILTER_FIFO0;
    filter.FilterIdHigh = 0U;
    filter.FilterIdLow = 0U;
    filter.FilterMaskIdHigh = 0U;
    filter.FilterMaskIdLow = 0U;

    if (HAL_CAN_ConfigFilter(handle, &filter) != HAL_OK) {
        return false;
    }
    if (HAL_CAN_Start(handle) != HAL_OK) {
        return false;
    }
    return HAL_CAN_ActivateNotification(handle, CAN_IT_RX_FIFO0_MSG_PENDING) == HAL_OK;
}

bool BspCan_Read(BspCanChannel channel, BspCanFrame *frame)
{
    CAN_HandleTypeDef *handle = handle_for(channel);
    if (!handle || !handle->Instance || !frame) {
        return false;
    }

    CAN_RxHeaderTypeDef header;
    uint8_t data[8] = {0};
    if (HAL_CAN_GetRxMessage(handle, CAN_RX_FIFO0, &header, data) != HAL_OK) {
        return false;
    }

    frame->standard_id = header.StdId;
    frame->length = header.DLC <= 8U ? (uint8_t)header.DLC : 8U;
    memcpy(frame->data, data, frame->length);
    frame->is_standard_frame = header.IDE == CAN_ID_STD;
    frame->is_data_frame = header.RTR == CAN_RTR_DATA;
    return true;
}

bool BspCan_Write(BspCanChannel channel,
                  uint16_t standard_id,
                  const uint8_t *data,
                  uint8_t length)
{
    CAN_HandleTypeDef *handle = handle_for(channel);
    if (!handle || !handle->Instance || length > 8U || (length > 0U && !data)) {
        return false;
    }

    CAN_TxHeaderTypeDef header = {0};
    uint8_t frame_data[8] = {0};
    uint32_t mailbox;
    header.StdId = standard_id;
    header.IDE = CAN_ID_STD;
    header.RTR = CAN_RTR_DATA;
    header.DLC = length;
    if (length > 0U) {
        memcpy(frame_data, data, length);
    }
    return HAL_CAN_AddTxMessage(handle, &header, frame_data, &mailbox) == HAL_OK;
}

bool BspCan_MatchesNativeHandle(BspCanChannel channel, const void *native_handle)
{
    return native_handle && native_handle == handle_for(channel);
}

bool BspCan_ReadDiagnostics(BspCanChannel channel, BspCanDiagnostics *diagnostics)
{
    if (!diagnostics) {
        return false;
    }

    CAN_HandleTypeDef *handle = handle_for(channel);
    if (!handle || !handle->Instance) {
        return false;
    }

    diagnostics->error = HAL_CAN_GetError(handle);
    diagnostics->error_status_register = handle->Instance->ESR;
    diagnostics->transmit_status_register = handle->Instance->TSR;
    diagnostics->receive_fifo0_register = handle->Instance->RF0R;
    diagnostics->free_tx_mailboxes = HAL_CAN_GetTxMailboxesFreeLevel(handle);
    return true;
}
