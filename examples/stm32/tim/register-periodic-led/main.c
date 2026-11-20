#include "stm32f4xx.h"

#define LED_MASK (1U << 2)

static volatile uint32_t updates;

static void timer_init(void)
{
  // 1. 打开 PB2 和 TIM2 的时钟并读回。LED 从低电平开始。
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
  (void)RCC->AHB1ENR;
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  (void)RCC->APB1ENR;
  GPIOB->BSRR = LED_MASK << 16;
  GPIOB->MODER = (GPIOB->MODER & ~(3U << 4)) | (1U << 4);

  // 2. HSI 经默认总线分频给 TIM2 约 16 MHz。分频后每 500 个计数更新一次。
  TIM2->PSC = 15999U;
  TIM2->ARR = 499U;
  TIM2->EGR = TIM_EGR_UG;

  // 3. 等手动更新把 PSC 装入，清掉这一标志。启动后的首个 IRQ 来自硬件计满。
  while ((TIM2->SR & TIM_SR_UIF) == 0U) {}
  TIM2->SR = 0U;
  NVIC_SetPriority(TIM2_IRQn, 6U);
  NVIC_EnableIRQ(TIM2_IRQn);
  TIM2->DIER = TIM_DIER_UIE;
  TIM2->CR1 = TIM_CR1_CEN;
}

void TIM2_IRQHandler(void)
{
  // 1. 只启用了更新中断。确认并清掉 UIF，再把本次更新变成一次 LED 翻转。
  if ((TIM2->SR & TIM_SR_UIF) != 0U)
  {
    TIM2->SR = 0U;
    ++updates;
    GPIOB->BSRR = (GPIOB->ODR & LED_MASK) ? LED_MASK << 16 : LED_MASK;
  }
}

int main(void)
{
  // 1. 完成引脚与计数器配置。之后 TIM2 IRQ 独立维护 LED 和计数。
  timer_init();
  for (;;)
  {
    // 2. main 等待中断。中断返回后回到这里等待下一次更新。
    __WFI();
  }
}
