#include <stdbool.h>
#include "stm32f4xx.h"

#define BUTTON_MASK (1U << 0)
#define LED_MASK (1U << 2)

static void gpio_init(void)
{
  // 1. 两个 GPIO 端口都要时钟。读回使能寄存器后再访问 GPIO。
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN;
  (void)RCC->AHB1ENR;

  // 2. PA0 配置为浮空输入，天空星的板载下拉定义松开时的电平。
  GPIOA->MODER &= ~(3U << (0U * 2U));
  GPIOA->PUPDR &= ~(3U << (0U * 2U));

  // 3. BSRR 先把 PB2 输出锁存值预置为低，再打开推挽输出。
  GPIOB->BSRR = LED_MASK << 16;
  GPIOB->OTYPER &= ~LED_MASK;
  GPIOB->OSPEEDR &= ~(3U << (2U * 2U));
  GPIOB->PUPDR &= ~(3U << (2U * 2U));
  GPIOB->MODER = (GPIOB->MODER & ~(3U << (2U * 2U))) | (1U << (2U * 2U));
}

int main(void)
{
  // 1. 系统启动代码交给 main 后，配置 PA0 与 PB2。
  gpio_init();

  for (;;)
  {
    // 2. 读本轮的 PA0 输入电平。主循环据此选择 PB2 的置位或复位命令。
    const bool pressed = (GPIOA->IDR & BUTTON_MASK) != 0U;

    // 3. 写 BSRR 低半字置位，写高半字复位。写完回到第 2 步继续采样。
    GPIOB->BSRR = pressed ? LED_MASK : LED_MASK << 16;
  }
}
