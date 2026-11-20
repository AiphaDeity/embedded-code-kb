#include <stdbool.h>
#include "stm32f4xx_hal.h"

enum { WORK_A = 1U, WORK_B = 2U, ALL_WORK = WORK_A | WORK_B, HEALTH_MS = 400U };

static IWDG_HandleTypeDef watchdog;
static uint32_t last_a, last_b, window_start, seen;
static volatile uint32_t work_a_steps, work_b_steps, refreshed, missed;
static volatile uint32_t reset_flags;
static volatile HAL_StatusTypeDef watchdog_status;

void SysTick_Handler(void) { HAL_IncTick(); }

static void work_a(uint32_t now)
{
  if ((uint32_t)(now - last_a) >= 50U)
  {
    // 1. 模拟一次短业务推进。完成后报告 A，窗口仍按 Monitor 的原起点计算。
    last_a = now;
    ++work_a_steps;
    seen |= WORK_A;
  }
}

static void work_b(uint32_t now)
{
  if ((uint32_t)(now - last_b) >= 100U)
  {
    last_b = now;
    // 1. 按住 PA0 模拟 B 无法完成。此时仍返回主循环，但本轮不报告进展。
    if (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)
    {
      ++work_b_steps;
      seen |= WORK_B;
    }
  }
}

static bool monitor_step(uint32_t now)
{
  if ((uint32_t)(now - window_start) < HEALTH_MS)
  {
    return true;
  }

  // 1. 在固定窗口结束处读取进展。缺一项就锁住故障，交给主循环停止喂狗。
  missed = ALL_WORK & ~seen;
  if (missed != 0U)
  {
    return false;
  }

  // 2. 两项均完成后才刷新。清除上一窗口的进展，再开始新的观察窗口。
  watchdog_status = HAL_IWDG_Refresh(&watchdog);
  if (watchdog_status != HAL_OK)
  {
    return false;
  }
  ++refreshed;
  seen = 0U;
  window_start = now;
  return true;
}

int main(void)
{
  // 1. 启动时先记录复位来源。IWDG 触发过复位时 LED 保持点亮，便于定位故障。
  HAL_Init();
  reset_flags = RCC->CSR;
  __HAL_RCC_CLEAR_RESET_FLAGS();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef key = {.Pin = GPIO_PIN_0, .Mode = GPIO_MODE_INPUT};
  GPIO_InitTypeDef led = {.Pin = GPIO_PIN_2, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOA, &key);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, reset_flags & RCC_CSR_IWDGRSTF ? GPIO_PIN_SET : GPIO_PIN_RESET);
  HAL_GPIO_Init(GPIOB, &led);

  // 2. 标称约 2 s 的 IWDG 给 400 ms 检查窗口留余量。初始化结束后从同一刻计时。
  watchdog.Instance = IWDG;
  watchdog.Init.Prescaler = IWDG_PRESCALER_32;
  watchdog.Init.Reload = 1999U;
  watchdog_status = HAL_IWDG_Init(&watchdog);
  if (watchdog_status != HAL_OK) { for (;;) {} }
  last_a = last_b = window_start = HAL_GetTick();

  for (;;)
  {
    const uint32_t now = HAL_GetTick();
    // 3. 到期先由 Monitor 结算上一窗口。此刻才推进的业务属于下一窗口。
    if (!monitor_step(now))
    {
      // 4. 故障已锁定。即使随后松键，也由 IWDG 超时复位后重新初始化。
      for (;;) { __WFI(); }
    }
    // 5. A 和 B 运行本轮业务，完成后再留下进展，由下一次到期检查读取。
    work_a(now);
    work_b(now);
    __WFI();
  }
}
