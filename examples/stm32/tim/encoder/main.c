#include "stm32f4xx_hal.h"

static TIM_HandleTypeDef encoder;
static volatile uint16_t count;
static volatile int64_t position;
static volatile HAL_StatusTypeDef setup_status;
void SysTick_Handler(void) { HAL_IncTick(); }

// 计数器为 16 位。两个采样之间的净计数须严格小于 32768。
static int32_t count_delta(uint16_t previous, uint16_t current)
{
  const uint16_t modulo = (uint16_t)(current - previous);
  return modulo < 32768U ? (int32_t)modulo : (int32_t)modulo - 65536;
}

static HAL_StatusTypeDef encoder_init(void)
{
  // 1. PA6/PA7 配为 TIM3 两路输入。上拉保持开路型编码器的空闲电平。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_TIM3_CLK_ENABLE();
  GPIO_InitTypeDef pins = {
    .Pin = GPIO_PIN_6 | GPIO_PIN_7, .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_PULLUP,
    .Speed = GPIO_SPEED_FREQ_LOW, .Alternate = GPIO_AF2_TIM3,
  };
  HAL_GPIO_Init(GPIOA, &pins);

  // 2. 两路输入的相位决定方向。计数器从 0 到 65535 循环。
  encoder.Instance = TIM3;
  encoder.Init = (TIM_Base_InitTypeDef){.CounterMode = TIM_COUNTERMODE_UP, .Period = UINT16_MAX};
  TIM_Encoder_InitTypeDef config = {
    .EncoderMode = TIM_ENCODERMODE_TI12,
    .IC1Polarity = TIM_ICPOLARITY_RISING, .IC1Selection = TIM_ICSELECTION_DIRECTTI,
    .IC1Prescaler = TIM_ICPSC_DIV1, .IC1Filter = 8U,
    .IC2Polarity = TIM_ICPOLARITY_RISING, .IC2Selection = TIM_ICSELECTION_DIRECTTI,
    .IC2Prescaler = TIM_ICPSC_DIV1, .IC2Filter = 8U,
  };
  if (HAL_TIM_Encoder_Init(&encoder, &config) != HAL_OK) { return HAL_ERROR; }
  return HAL_TIM_Encoder_Start(&encoder, TIM_CHANNEL_ALL);
}

int main(void)
{
  // 1. 启用编码器模式，捕获起始计数。之后所有有效边沿由 TIM3 计数。
  HAL_Init();
  setup_status = encoder_init();
  if (setup_status != HAL_OK) { for (;;) {} }
  uint16_t previous = (uint16_t)__HAL_TIM_GET_COUNTER(&encoder);

  for (;;)
  {
    // 2. 主循环每隔约 10 ms 读取一次，用模差求有符号的移动量。
    HAL_Delay(10U);
    const uint16_t current = (uint16_t)__HAL_TIM_GET_COUNTER(&encoder);
    position += count_delta(previous, current);
    count = current;
    previous = current;
  }
}
