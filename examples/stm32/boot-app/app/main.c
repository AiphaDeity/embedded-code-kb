#include "stm32f4xx.h"

#define LED_MASK (1U << 2)

static volatile uint32_t ticks;
static volatile uint32_t half_period_ms = 250U;
static volatile uint32_t toggles;

void SysTick_Handler(void)
{
  ++ticks;
}

int main(void)
{
  // 1. App 的 Reset_Handler 已复制 .data、清零 .bss。配置板载 LED 与自己的 SysTick。
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
  (void)RCC->AHB1ENR;
  GPIOB->BSRR = LED_MASK << 16;
  GPIOB->MODER = (GPIOB->MODER & ~(3U << 4)) | (1U << 4);
  if (SysTick_Config(SystemCoreClock / 1000U) != 0U)
  {
    for (;;) {}
  }

  uint32_t last = ticks;
  for (;;)
  {
    // 2. App 的时基从零重新计数。每到半周期就翻灯，供观察 Boot 是否真正完成交接。
    const uint32_t now = ticks;
    if ((uint32_t)(now - last) >= half_period_ms)
    {
      last = now;
      GPIOB->BSRR = (GPIOB->ODR & LED_MASK) ? LED_MASK << 16 : LED_MASK;
      ++toggles;
    }

    // 3. 等自己的下一个中断节拍，再检查是否要更新 LED。
    __WFI();
  }
}
