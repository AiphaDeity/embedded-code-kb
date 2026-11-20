#include "stm32f4xx_hal.h"

static IWDG_HandleTypeDef watchdog;
static volatile uint32_t reset_flags;
static volatile HAL_StatusTypeDef watchdog_status;

void SysTick_Handler(void) { HAL_IncTick(); }

int main(void)
{
  // 1. 读取本次启动的复位标志，再清除累计标志。
  HAL_Init();
  reset_flags = RCC->CSR;
  __HAL_RCC_CLEAR_RESET_FLAGS();

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef key = {.Pin = GPIO_PIN_0, .Mode = GPIO_MODE_INPUT};
  GPIO_InitTypeDef led = {.Pin = GPIO_PIN_2, .Mode = GPIO_MODE_OUTPUT_PP};
  const int watchdog_reset = (reset_flags & RCC_CSR_IWDGRSTF) != 0U;
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, watchdog_reset ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_Init(GPIOA, &key);
  HAL_GPIO_Init(GPIOB, &led);

  // 2. 配置独立看门狗。配置结束后立即进入喂狗循环。
  watchdog.Instance = IWDG;
  watchdog.Init.Prescaler = IWDG_PRESCALER_32;
  watchdog.Init.Reload = 1999U;
  watchdog_status = HAL_IWDG_Init(&watchdog);
  if (watchdog_status != HAL_OK) { for (;;) {} }

  for (;;)
  {
    // 3. 首轮实验遇到按键后停喂。复位后保留 LED 提示并持续喂狗。
    if (!watchdog_reset && HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET)
    {
      for (;;) {}
    }
    watchdog_status = HAL_IWDG_Refresh(&watchdog);
    HAL_Delay(100U);
  }
}
