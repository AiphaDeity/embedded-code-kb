#ifndef SKY_ADC1_PC0_H
#define SKY_ADC1_PC0_H

#include "stm32f4xx_hal.h"

// 天空星 PC0 / ADC1 通道 10。单通道、单次软件触发，单位为 12 位原始计数。
static inline HAL_StatusTypeDef sky_adc1_pc0_init(ADC_HandleTypeDef *adc)
{
  // 1. 先打开 GPIOC 和 ADC1 时钟。PC0 进入模拟模式后由采样电路接管。
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_ADC1_CLK_ENABLE();
  GPIO_InitTypeDef pin = {.Pin = GPIO_PIN_0, .Mode = GPIO_MODE_ANALOG};
  HAL_GPIO_Init(GPIOC, &pin);

  // 2. 配置 ADC1 的规则通道 10。初始化失败时将状态交回调用者。
  adc->Instance = ADC1;
  adc->Init = (ADC_InitTypeDef){
    .ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4, .Resolution = ADC_RESOLUTION_12B,
    .ScanConvMode = DISABLE, .ContinuousConvMode = DISABLE,
    .ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE,
    .ExternalTrigConv = ADC_SOFTWARE_START, .DataAlign = ADC_DATAALIGN_RIGHT,
    .NbrOfConversion = 1U, .EOCSelection = ADC_EOC_SINGLE_CONV,
  };
  ADC_ChannelConfTypeDef channel = {
    .Channel = ADC_CHANNEL_10, .Rank = 1U, .SamplingTime = ADC_SAMPLETIME_144CYCLES,
  };
  const HAL_StatusTypeDef status = HAL_ADC_Init(adc);
  return status == HAL_OK ? HAL_ADC_ConfigChannel(adc, &channel) : status;
}

#endif
