#include "stm32f4xx_hal.h"

#define DEBOUNCE_MS 30U
static volatile uint32_t edges;
static volatile uint32_t accepted;

void SysTick_Handler(void) { HAL_IncTick(); }
void EXTI0_IRQHandler(void) { HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0); }

void HAL_GPIO_EXTI_Callback(uint16_t pin)
{
  // 1. HAL 已清这条线的挂起位。回调只累计边沿，主循环负责防抖和翻转 LED。
  if (pin == GPIO_PIN_0) { ++edges; }
}

int main(void)
{
  // 1. 初始化 LED 和 PA0 上升沿输入。
  HAL_Init();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  GPIO_InitTypeDef led = {.Pin = GPIO_PIN_2, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOB, &led);
  GPIO_InitTypeDef key = {.Pin = GPIO_PIN_0, .Mode = GPIO_MODE_IT_RISING};
  HAL_GPIO_Init(GPIOA, &key);

  // 2. 主循环的记录准备完毕后才开放 NVIC。先清除配置阶段遗留的 pending。
  uint32_t seen = 0;
  uint32_t last_edge_ms = HAL_GetTick() - DEBOUNCE_MS;
  __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_0);
  HAL_NVIC_ClearPendingIRQ(EXTI0_IRQn);
  HAL_NVIC_SetPriority(EXTI0_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(EXTI0_IRQn);

  for (;;)
  {
    // 3. 回调是计数的唯一写入者。主循环读取快照，过滤时间上过近的边沿。
    const uint32_t count = edges;
    if (count == seen) { continue; }
    seen = count;
    const uint32_t now = HAL_GetTick();
    if (now - last_edge_ms >= DEBOUNCE_MS)
    {
      ++accepted;
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
    }
    last_edge_ms = now;
  }
}
