#ifndef SKY_USART1_H
#define SKY_USART1_H

#include "stm32f4xx_hal.h"

// 调用前已初始化 HAL。配置天空星 PA9 / PA10 为 115200、8N1、无流控。
static inline HAL_StatusTypeDef sky_usart1_init(UART_HandleTypeDef *uart)
{
  // 1. USART1 与 GPIOA 分属不同的总线。使能后由 AF7 接管两个引脚。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();
  GPIO_InitTypeDef pins = {
    .Pin = GPIO_PIN_9 | GPIO_PIN_10, .Mode = GPIO_MODE_AF_PP,
    .Pull = GPIO_PULLUP, .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF7_USART1,
  };
  HAL_GPIO_Init(GPIOA, &pins);

  // 2. 只配置外设。调用者准备好接收缓冲和中断再开始传输。
  uart->Instance = USART1;
  uart->Init = (UART_InitTypeDef){
    .BaudRate = 115200U, .WordLength = UART_WORDLENGTH_8B,
    .StopBits = UART_STOPBITS_1, .Parity = UART_PARITY_NONE,
    .Mode = UART_MODE_TX_RX, .HwFlowCtl = UART_HWCONTROL_NONE,
    .OverSampling = UART_OVERSAMPLING_16,
  };
  return HAL_UART_Init(uart);
}

#endif
