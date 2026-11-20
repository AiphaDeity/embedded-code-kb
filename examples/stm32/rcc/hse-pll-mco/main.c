#include "stm32f4xx_hal.h"

typedef struct
{
  uint32_t cr, cfgr, pllcfgr, flash_acr;
  uint32_t sysclk_hz, hclk_hz, pclk1_hz, pclk2_hz, systick_load;
} ClockSnapshot;

static volatile ClockSnapshot after_reset, after_pll_lock, after_switch;
static volatile HAL_StatusTypeDef oscillator_status, switch_status;
static volatile uint32_t heartbeat;

void SysTick_Handler(void) { HAL_IncTick(); }

static ClockSnapshot capture(void)
{
  return (ClockSnapshot){
    .cr = RCC->CR,
    .cfgr = RCC->CFGR,
    .pllcfgr = RCC->PLLCFGR,
    .flash_acr = FLASH->ACR,
    .sysclk_hz = HAL_RCC_GetSysClockFreq(),
    .hclk_hz = HAL_RCC_GetHCLKFreq(),
    .pclk1_hz = HAL_RCC_GetPCLK1Freq(),
    .pclk2_hz = HAL_RCC_GetPCLK2Freq(),
    .systick_load = SysTick->LOAD,
  };
}

int main(void)
{
  // 1. HAL 先用复位后的 HSI 建立毫秒时基。记录旧时钟，再提前选好 PA8 的输出。
  HAL_Init();
  after_reset = capture();
#ifdef SKY_MCO_PLL
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_PLLCLK, RCC_MCODIV_4);
#else
  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);
#endif

  // 2. 先等晶振与 PLL 就绪。此时 SYSCLK 仍用 HSI，启动失败就停下供调试。
  const RCC_OscInitTypeDef clock_source = {
    .OscillatorType = RCC_OSCILLATORTYPE_HSE,
    .HSEState = RCC_HSE_ON,
    .PLL = {
      .PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSE,
      .PLLM = 8U, .PLLN = 192U, .PLLP = RCC_PLLP_DIV4, .PLLQ = 4U,
    },
  };
  oscillator_status = HAL_RCC_OscConfig(&clock_source);
  after_pll_lock = capture();
  if (oscillator_status != HAL_OK)
  {
    for (;;) { __WFI(); }
  }

  // 3. HAL 先提高 Flash 等待周期，再切到 PLL 并按新的 HCLK 更新 SysTick。
  const RCC_ClkInitTypeDef clocks = {
    .ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                 RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
    .SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK,
    .AHBCLKDivider = RCC_SYSCLK_DIV1,
    .APB1CLKDivider = RCC_HCLK_DIV2,
    .APB2CLKDivider = RCC_HCLK_DIV1,
  };
  switch_status = HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_1);
  after_switch = capture();
  if (switch_status != HAL_OK)
  {
    for (;;) { __WFI(); }
  }

  for (;;)
  {
    // 4. 切换成功后继续由 SysTick 提供毫秒节拍。用计数确认主循环仍在推进。
    ++heartbeat;
    HAL_Delay(1000U);
  }
}
