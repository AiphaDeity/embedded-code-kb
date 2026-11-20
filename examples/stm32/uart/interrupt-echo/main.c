#include "stm32f4xx_hal.h"
#include <stdbool.h>

static UART_HandleTypeDef uart;
static uint8_t rx_byte, tx_byte;
static volatile uint8_t pending_byte;
static volatile bool rx_arm = true;
static volatile bool rx_fault, pending, tx_busy;
static volatile uint32_t loop_count, echo_count, dropped_bytes, rx_errors, tx_start_failures;

static void uart_init(void)
{
  // 1. 给端口和 USART1 打开时钟，让 PA9/PA10 使用 AF7。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();
  GPIO_InitTypeDef pins = {
    .Pin = GPIO_PIN_9 | GPIO_PIN_10, .Mode = GPIO_MODE_AF_PP,
    .Pull = GPIO_PULLUP, .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF7_USART1,
  };
  HAL_GPIO_Init(GPIOA, &pins);

  // 2. 完成 UART 初始化后再允许 USART1 IRQ。
  uart.Instance = USART1;
  uart.Init = (UART_InitTypeDef){
    .BaudRate = 115200U, .WordLength = UART_WORDLENGTH_8B, .StopBits = UART_STOPBITS_1,
    .Parity = UART_PARITY_NONE, .Mode = UART_MODE_TX_RX,
    .HwFlowCtl = UART_HWCONTROL_NONE, .OverSampling = UART_OVERSAMPLING_16,
  };
  if (HAL_UART_Init(&uart) != HAL_OK) { for (;;) {} }
  HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void SysTick_Handler(void) { HAL_IncTick(); }
void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&uart); }

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *handle)
{
  if (handle != &uart) { return; }
  // 1. 交给主循环自己的副本，然后通知主循环重挂接收。
  if (!pending)
  {
    pending_byte = rx_byte;
    pending = true;
  }
  else
  {
    ++dropped_bytes;
  }
  rx_arm = true;
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *handle)
{
  if (handle == &uart)
  {
    // 1. USART1 完成发送后记一次回显。清除忙标志，允许 main() 改写 tx_byte。
    ++echo_count;
    tx_busy = false;
  }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *handle)
{
  if (handle == &uart)
  {
    // 1. 记下接收错误并通知 main()。主循环会丢弃原接收槽并重挂接收。
    ++rx_errors;
    // HAL 可能在回调返回后还更新状态，等 IRQ 结束再由主循环恢复。
    rx_fault = true;
  }
}

int main(void)
{
  // 1. 初始化时基、引脚和 USART1 中断入口。
  HAL_Init();
  uart_init();
  for (;;)
  {
    ++loop_count;
    // 2. 短暂屏蔽中断后处理交接。三个 HAL 调用均在本例的无 DMA 路径运行。
    __disable_irq();
    // 出错时停掉残留接收并清标志，丢掉接收槽中的旧字节。
    if (rx_fault)
    {
      HAL_UART_AbortReceive(&uart);
      __HAL_UART_CLEAR_OREFLAG(&uart);
      pending = false;
      rx_fault = false;
      rx_arm = true;
    }
    // 首轮、上一字节收完或错误恢复后都从这里挂接收。失败时保留 rx_arm，下轮再试。
    if (rx_arm && HAL_UART_Receive_IT(&uart, &rx_byte, 1U) == HAL_OK)
    {
      rx_arm = false;
    }

    // 3. TX 完成回调释放缓冲后，才填入下一字节。若启动失败，保留接收槽，下轮重试。
    if (pending && !tx_busy)
    {
      tx_byte = pending_byte;
      if (HAL_UART_Transmit_IT(&uart, &tx_byte, 1U) == HAL_OK)
      {
        tx_busy = true;
        pending = false;
      }
      else
      {
        ++tx_start_failures;
      }
    }
    // 4. 恢复中断，让串口的 RX 和 TX 事件推进。
    __enable_irq();
  }
}
