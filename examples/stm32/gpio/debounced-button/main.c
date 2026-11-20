#include "stm32f4xx_hal.h"
#include "../../../c/debounce/debounce.h"

static volatile uint32_t presses, releases, heartbeat;
void SysTick_Handler(void) { HAL_IncTick(); }

int main(void)
{
  // 1. PA0 由板载下拉定义空闲电平。PB2 用按下事件切换。
  HAL_Init();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef key = {.Pin = GPIO_PIN_0, .Mode = GPIO_MODE_INPUT};
  GPIO_InitTypeDef led = {.Pin = GPIO_PIN_2, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOA, &key);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  HAL_GPIO_Init(GPIOB, &led);
  Debounce button = {0};
  uint32_t last_heartbeat = HAL_GetTick();

  for (;;)
  {
    // 2. 本轮采样后立即继续。按下稳定满 20 ms 才翻转一次 LED。
    const uint32_t now = HAL_GetTick();
    const bool raw = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET;
    const KeyEvent event = debounce_update(&button, raw, now, 20U);
    if (event == KEY_PRESSED)
    {
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
      ++presses;
    }
    else if (event == KEY_RELEASED)
    {
      ++releases;
    }

    // 3. 消抖等待期间仍运行其他逻辑。这里让调试计数每 100 ms 加一。
    if ((uint32_t)(now - last_heartbeat) >= 100U)
    {
      last_heartbeat = now;
      ++heartbeat;
    }
    __WFI();
  }
}
