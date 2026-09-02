/*
 * 配置 DR16 接收使用的 USART3 和 DMA，并在空闲中断时交出完整缓冲区。
 * 中断中只切换缓冲区和通知解析模块，不处理机器人模式。
 */
#include "bsp_rc.h"
#include "main.h"

extern UART_HandleTypeDef huart3;
extern DMA_HandleTypeDef hdma_usart3_rx;

static uint8_t *s_rx_buffers[2];
static uint16_t s_dma_buffer_size;
static BspRcFrameCallback s_frame_callback;

void BspRc_SetFrameCallback(BspRcFrameCallback callback)
{
    s_frame_callback = callback;
}

void RC_init(uint8_t *rx1_buf, uint8_t *rx2_buf, uint16_t dma_buf_num)
{
    if (!rx1_buf || !rx2_buf || dma_buf_num == 0U) {
        return;
    }

    s_rx_buffers[0] = rx1_buf;
    s_rx_buffers[1] = rx2_buf;
    s_dma_buffer_size = dma_buf_num;

    SET_BIT(huart3.Instance->CR3, USART_CR3_DMAR);
    __HAL_DMA_DISABLE(&hdma_usart3_rx);
    while (hdma_usart3_rx.Instance->CR & DMA_SxCR_EN) {
        __HAL_DMA_DISABLE(&hdma_usart3_rx);
    }

    hdma_usart3_rx.Instance->PAR = (uint32_t)&USART3->DR;
    hdma_usart3_rx.Instance->M0AR = (uint32_t)rx1_buf;
    hdma_usart3_rx.Instance->M1AR = (uint32_t)rx2_buf;
    hdma_usart3_rx.Instance->NDTR = dma_buf_num;
    SET_BIT(hdma_usart3_rx.Instance->CR, DMA_SxCR_DBM);
    __HAL_DMA_ENABLE(&hdma_usart3_rx);

    __HAL_UART_CLEAR_IDLEFLAG(&huart3);
    __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
}

void REMOTE_USART3_IDLE_IRQHandler(void)
{
    if (huart3.Instance->SR & UART_FLAG_RXNE) {
        __HAL_UART_CLEAR_PEFLAG(&huart3);
        return;
    }
    if (!(USART3->SR & UART_FLAG_IDLE) || s_dma_buffer_size == 0U) {
        return;
    }

    __HAL_UART_CLEAR_PEFLAG(&huart3);

    uint8_t completed_index;
    if ((hdma_usart3_rx.Instance->CR & DMA_SxCR_CT) == RESET) {
        completed_index = 0U;
        __HAL_DMA_DISABLE(&hdma_usart3_rx);
        uint16_t received = s_dma_buffer_size - hdma_usart3_rx.Instance->NDTR;
        hdma_usart3_rx.Instance->NDTR = s_dma_buffer_size;
        hdma_usart3_rx.Instance->CR |= DMA_SxCR_CT;
        __HAL_DMA_ENABLE(&hdma_usart3_rx);
        if (s_frame_callback) {
            s_frame_callback(s_rx_buffers[completed_index], received);
        }
    } else {
        completed_index = 1U;
        __HAL_DMA_DISABLE(&hdma_usart3_rx);
        uint16_t received = s_dma_buffer_size - hdma_usart3_rx.Instance->NDTR;
        hdma_usart3_rx.Instance->NDTR = s_dma_buffer_size;
        DMA1_Stream1->CR &= ~DMA_SxCR_CT;
        __HAL_DMA_ENABLE(&hdma_usart3_rx);
        if (s_frame_callback) {
            s_frame_callback(s_rx_buffers[completed_index], received);
        }
    }
}
