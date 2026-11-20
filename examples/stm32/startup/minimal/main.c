#include "stm32f4xx.h"
#include <stdbool.h>

enum { EXPECTED_COOKIE = 0x1234ABCDU, LED = 1U << 2, PERIOD_MS = 500U };

static volatile uint32_t boot_cookie = EXPECTED_COOKIE;
static volatile uint32_t zero_cookie, startup_stage, tick_ms, led_toggles;

static void led_init(void)
{
  // 1. 打开 GPIOB 时钟，先预置低电平，再把 PB2 切换为推挽输出。
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
  (void)RCC->AHB1ENR;
  GPIOB->BSRR = LED << 16;
  GPIOB->MODER = (GPIOB->MODER & ~(3U << 4)) | (1U << 4);
}

static void stop_at(uint32_t stage)
{
  // 1. 留下失败阶段。此时没有成功启动节拍，主循环不再更新 LED。
  startup_stage = stage;
  for (;;) { __WFI(); }
}

void SysTick_Handler(void)
{
  // 1. 中断只发布累计毫秒数，LED 由主循环负责。
  ++tick_ms;
}

int main(void)
{
  // 1. Reset_Handler 已完成 .data 复制和 .bss 清零。可在 GPIO 初始化前核对它们。
  if (boot_cookie != EXPECTED_COOKIE || zero_cookie != 0U) { stop_at(0xE1U); }
  startup_stage = 1U;

  // 2. 先初始化 LED，再每毫秒中断一次。定时器配置失败时保留诊断状态。
  led_init();
  startup_stage = 2U;
  if (SysTick_Config(SystemCoreClock / 1000U) != 0U) { stop_at(0xE2U); }
  startup_stage = 3U;

  uint32_t last_toggle = tick_ms;
  bool led_on = false;
  for (;;)
  {
    // 3. 只在到达 500 ms 时翻转。用无符号差判断间隔，tick_ms 回绕也能继续。
    const uint32_t now = tick_ms;
    if (now - last_toggle >= PERIOD_MS)
    {
      last_toggle = now;
      led_on = !led_on;
      GPIOB->BSRR = led_on ? LED : (LED << 16);
      ++led_toggles;
    }
    __WFI();
  }
}
