#ifndef SKY_CLOCK84_H
#define SKY_CLOCK84_H

#include "stm32f4xx_hal.h"

// STM32F407 在板上正常供电时从 HSI 配置 SYSCLK 84 MHz，PLLQ 提供标称 48 MHz。
static inline HAL_StatusTypeDef sky_clock84(void)
{
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  // 1. 16 MHz / 16 × 336 得到 PLL VCO。P=4 供内核，Q=7 供 SDIO。
  RCC_OscInitTypeDef osc = {
    .OscillatorType = RCC_OSCILLATORTYPE_HSI, .HSIState = RCC_HSI_ON,
    .HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT,
    .PLL = {.PLLState = RCC_PLL_ON, .PLLSource = RCC_PLLSOURCE_HSI,
            .PLLM = 16U, .PLLN = 336U, .PLLP = RCC_PLLP_DIV4, .PLLQ = 7U},
  };
  const HAL_StatusTypeDef status = HAL_RCC_OscConfig(&osc);
  if (status != HAL_OK)
  {
    return status;
  }

  // 2. 先由 HAL 设置 Flash 等待周期，再切 SYSCLK 和总线。HAL 同步更新 SysTick。
  RCC_ClkInitTypeDef clocks = {
    .ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
    .SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK, .AHBCLKDivider = RCC_SYSCLK_DIV1,
    .APB1CLKDivider = RCC_HCLK_DIV2, .APB2CLKDivider = RCC_HCLK_DIV1,
  };
  return HAL_RCC_ClockConfig(&clocks, FLASH_LATENCY_2);
}

#endif
