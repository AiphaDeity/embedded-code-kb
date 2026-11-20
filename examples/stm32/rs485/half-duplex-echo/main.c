#include "stm32f4xx_hal.h"

#define DIR_PIN GPIO_PIN_8
static UART_HandleTypeDef uart;
static volatile HAL_StatusTypeDef status;
static volatile uint32_t echoed_bytes, transmit_errors;

void SysTick_Handler(void) { HAL_IncTick(); }

int main(void)
{
  // 1. 初始化方向脚为接收，UART 再接管 PA9 和 PA10。
  HAL_Init();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOA, DIR_PIN, GPIO_PIN_RESET);
  GPIO_InitTypeDef direction = {.Pin = DIR_PIN, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOA, &direction);
  GPIO_InitTypeDef pins = {
    .Pin = GPIO_PIN_9 | GPIO_PIN_10, .Mode = GPIO_MODE_AF_PP,
    .Pull = GPIO_PULLUP, .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF7_USART1,
  };
  HAL_GPIO_Init(GPIOA, &pins);
  uart.Instance = USART1;
  uart.Init = (UART_InitTypeDef){
    .BaudRate = 9600U, .WordLength = UART_WORDLENGTH_8B, .StopBits = UART_STOPBITS_1,
    .Parity = UART_PARITY_NONE, .Mode = UART_MODE_TX_RX,
    .HwFlowCtl = UART_HWCONTROL_NONE, .OverSampling = UART_OVERSAMPLING_16,
  };
  status = HAL_UART_Init(&uart);
  if (status != HAL_OK) { for (;;) {} }

  for (;;)
  {
    // 2. 轮询接收。收到一个完整字节后，给自动方向适配器留出释放时间。
    uint8_t value;
    if (HAL_UART_Receive(&uart, &value, 1U, 100U) != HAL_OK) { continue; }
    HAL_Delay(3U);

    // 3. 发送期间保持 DE 有效。HAL 成功路径返回时 TC 已置位。
    HAL_GPIO_WritePin(GPIOA, DIR_PIN, GPIO_PIN_SET);
    status = HAL_UART_Transmit(&uart, &value, 1U, 20U);
    // 4. 发送无论是否成功都切回接收，错误时放弃当前回显。
    HAL_GPIO_WritePin(GPIOA, DIR_PIN, GPIO_PIN_RESET);
    if (status == HAL_OK) { ++echoed_bytes; }
    else { ++transmit_errors; }
  }
}
