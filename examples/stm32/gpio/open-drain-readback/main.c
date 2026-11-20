#include "stm32f4xx_hal.h"

#define SIGNAL_PIN GPIO_PIN_1

typedef struct
{
  GPIO_PinState output_latch;
  GPIO_PinState input_level;
} PinSample;

static volatile PinSample after_drive_low, after_release;
static volatile uint32_t cycles;

void SysTick_Handler(void) { HAL_IncTick(); }

// ODR 表示软件写入的值。IDR 取自同一焊盘的输入路径。
static PinSample sample_pin(void)
{
  return (PinSample){
    .output_latch = (GPIOA->ODR & SIGNAL_PIN) ? GPIO_PIN_SET : GPIO_PIN_RESET,
    .input_level = HAL_GPIO_ReadPin(GPIOA, SIGNAL_PIN),
  };
}

int main(void)
{
  // 1. 先将 PA1 的输出锁存器置 1，再切到开漏。此时由外部电阻拉高线路。
  HAL_Init();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOA, SIGNAL_PIN, GPIO_PIN_SET);
  GPIO_InitTypeDef signal = {
    .Pin = SIGNAL_PIN, .Mode = GPIO_MODE_OUTPUT_OD, .Pull = GPIO_NOPULL,
    .Speed = GPIO_SPEED_FREQ_LOW,
  };
  HAL_GPIO_Init(GPIOA, &signal);

  for (;;)
  {
    // 2. 将线路拉低，等电平稳定，再记录锁存器与实际输入。
    HAL_GPIO_WritePin(GPIOA, SIGNAL_PIN, GPIO_PIN_RESET);
    HAL_Delay(10U);
    after_drive_low = sample_pin();
    HAL_Delay(490U);

    // 3. 释放线路并再次回读。外部按键若仍把线拉低，ODR 为 1 且 IDR 为 0。
    HAL_GPIO_WritePin(GPIOA, SIGNAL_PIN, GPIO_PIN_SET);
    HAL_Delay(10U);
    after_release = sample_pin();
    ++cycles;
    HAL_Delay(490U);
  }
}
