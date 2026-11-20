#include "../../common/sky-usart1.h"
#include "rx_ring.h"

static UART_HandleTypeDef uart;
static UartRing ring;
static uint8_t rx_byte;
static volatile bool rx_restart;
static volatile uint32_t rx_queued, ring_full_drops, rx_errors, echoed, tx_failures;

void SysTick_Handler(void) { HAL_IncTick(); }
void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&uart); }

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *handle)
{
  if (handle != &uart || HAL_UART_GetError(handle) != HAL_UART_ERROR_NONE)
  {
    return;
  }

  // 1. 字节先进入环形缓冲区。没有空槽时丢这个新字节，旧字节由主循环继续处理。
  if (uart_ring_push(&ring, rx_byte)) { ++rx_queued; }
  else { ++ring_full_drops; }

  // 2. HAL 此时已释放本次接收状态。立即用同一个硬件接收槽挂起下一字节。
  if (HAL_UART_Receive_IT(handle, &rx_byte, 1U) != HAL_OK) { rx_restart = true; }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *handle)
{
  if (handle == &uart)
  {
    // 1. 留给主循环在 HAL 退出 IRQ 后重启，已入队的字节仍保持顺序。
    ++rx_errors;
    rx_restart = true;
  }
}

static void restart_rx_if_needed(void)
{
  if (!rx_restart) { return; }

  // 1. 仅在无 DMA 的接收路径短暂屏蔽中断。恢复 HAL 状态并丢弃硬件中的残留字节。
  __disable_irq();
  if (HAL_UART_AbortReceive(&uart) == HAL_OK)
  {
    __HAL_UART_CLEAR_OREFLAG(&uart);
    rx_restart = HAL_UART_Receive_IT(&uart, &rx_byte, 1U) != HAL_OK;
  }
  __enable_irq();
}

int main(void)
{
  // 1. 先配置 USART1 和第一字节的接收，再开放 NVIC。
  HAL_Init();
  if (sky_usart1_init(&uart) != HAL_OK ||
      HAL_UART_Receive_IT(&uart, &rx_byte, 1U) != HAL_OK)
  {
    for (;;) {}
  }
  HAL_NVIC_SetPriority(USART1_IRQn, 6U, 0U);
  HAL_NVIC_EnableIRQ(USART1_IRQn);

  for (;;)
  {
    // 2. 先处理接收错误。没有数据就继续等待，不访问中断正在写入的槽。
    restart_rx_if_needed();
    uint8_t byte;
    if (!uart_ring_pop(&ring, &byte)) { continue; }

    // 3. 手里的字节是副本，原槽已经归还。发送有 10 ms 期限，期间 RX 中断仍可运行。
    if (HAL_UART_Transmit(&uart, &byte, 1U, 10U) == HAL_OK) { ++echoed; }
    else { ++tx_failures; }
  }
}
