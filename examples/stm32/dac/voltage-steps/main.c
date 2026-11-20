#include "stm32f4xx_hal.h"

static DAC_HandleTypeDef dac;
static volatile HAL_StatusTypeDef status;
static volatile uint16_t active_code;

void SysTick_Handler(void) { HAL_IncTick(); }

int main(void)
{
  // 1. 给通道 1 准备 PA4 模拟引脚。
  HAL_Init();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_DAC_CLK_ENABLE();
  GPIO_InitTypeDef pin = {.Pin = GPIO_PIN_4, .Mode = GPIO_MODE_ANALOG};
  HAL_GPIO_Init(GPIOA, &pin);

  // 2. 选无触发模式。写入数据寄存器后由硬件更新输出。
  dac.Instance = DAC;
  DAC_ChannelConfTypeDef channel = {
    .DAC_Trigger = DAC_TRIGGER_NONE, .DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE,
  };
  status = HAL_DAC_Init(&dac);
  if (status != HAL_OK) { for (;;) {} }
  status = HAL_DAC_ConfigChannel(&dac, &channel, DAC_CHANNEL_1);
  if (status != HAL_OK) { for (;;) {} }
  status = HAL_DAC_Start(&dac, DAC_CHANNEL_1);
  if (status != HAL_OK) { for (;;) {} }
  const uint16_t codes[] = {1024, 2048, 3072};

  for (;;)
  {
    // 3. 每次写入一个 12 位值，等电压稳定后再切换下一档。
    for (uint32_t i = 0; i < sizeof(codes) / sizeof(codes[0]); ++i)
    {
      status = HAL_DAC_SetValue(&dac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, codes[i]);
      if (status != HAL_OK) { for (;;) {} }
      active_code = codes[i];
      HAL_Delay(1000U);
    }
  }
}
