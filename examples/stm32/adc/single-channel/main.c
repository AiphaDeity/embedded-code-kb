#include "../../common/sky-adc1-pc0.h"

#define REFERENCE_MV 3000U
static ADC_HandleTypeDef adc;
static volatile HAL_StatusTypeDef status;
static volatile uint32_t raw, millivolts;

void SysTick_Handler(void) { HAL_IncTick(); }

int main(void)
{
  // 1. 初始化时基，再让 PC0 接入 ADC1 的规则通道 10。
  HAL_Init();
  status = sky_adc1_pc0_init(&adc);
  if (status != HAL_OK) { for (;;) {} }

  for (;;)
  {
    // 2. 确认本轮完成后才读 DR。发生超时就停止本轮转换，下一轮重新启动。
    status = HAL_ADC_Start(&adc);
    if (status == HAL_OK)
    {
      status = HAL_ADC_PollForConversion(&adc, 20U);
      if (status == HAL_OK)
      {
        const uint32_t value = HAL_ADC_GetValue(&adc);
        raw = value;
        millivolts = (value * REFERENCE_MV + 2047U) / 4095U;
      }
      HAL_ADC_Stop(&adc);
    }
    HAL_Delay(100U);
  }
}
