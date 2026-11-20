#include "stm32f4xx_hal.h"

static ADC_HandleTypeDef adc;
static UART_HandleTypeDef uart;
static volatile uint32_t sent_frames, sample_errors, serial_errors;

void SysTick_Handler(void) { HAL_IncTick(); }

static void setup_peripherals(void)
{
  // 1. PC0 接 ADC1 通道 10。PA9 作为 USART1 发送输出。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_ADC1_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();
  GPIO_InitTypeDef analog = {.Pin = GPIO_PIN_0, .Mode = GPIO_MODE_ANALOG};
  HAL_GPIO_Init(GPIOC, &analog);
  GPIO_InitTypeDef tx = {
    .Pin = GPIO_PIN_9, .Mode = GPIO_MODE_AF_PP,
    .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF7_USART1,
  };
  HAL_GPIO_Init(GPIOA, &tx);

  // 2. 每次软件触发只产生一个 12 位结果。
  adc.Instance = ADC1;
  adc.Init = (ADC_InitTypeDef){
    .ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4, .Resolution = ADC_RESOLUTION_12B,
    .ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE, .ExternalTrigConv = ADC_SOFTWARE_START,
    .DataAlign = ADC_DATAALIGN_RIGHT, .NbrOfConversion = 1U, .EOCSelection = ADC_EOC_SINGLE_CONV,
  };
  ADC_ChannelConfTypeDef channel = {
    .Channel = ADC_CHANNEL_10, .Rank = 1U, .SamplingTime = ADC_SAMPLETIME_144CYCLES,
  };
  if (HAL_ADC_Init(&adc) != HAL_OK || HAL_ADC_ConfigChannel(&adc, &channel) != HAL_OK)
  {
    ++sample_errors;
    for (;;) {}
  }

  // 3. 发送端完成初始化后才进入采样循环。
  uart.Instance = USART1;
  uart.Init = (UART_InitTypeDef){
    .BaudRate = 115200U, .WordLength = UART_WORDLENGTH_8B, .StopBits = UART_STOPBITS_1,
    .Parity = UART_PARITY_NONE, .Mode = UART_MODE_TX, .HwFlowCtl = UART_HWCONTROL_NONE,
    .OverSampling = UART_OVERSAMPLING_16,
  };
  if (HAL_UART_Init(&uart) != HAL_OK)
  {
    ++serial_errors;
    for (;;) {}
  }
}

static void sample_and_send(void)
{
  // 1. 只有本轮转换完成，才采纳数据寄存器里的值。
  if (HAL_ADC_Start(&adc) != HAL_OK)
  {
    ++sample_errors;
    return;
  }
  if (HAL_ADC_PollForConversion(&adc, 20U) != HAL_OK)
  {
    HAL_ADC_Stop(&adc);
    ++sample_errors;
    return;
  }
  const uint16_t raw = (uint16_t)HAL_ADC_GetValue(&adc);
  HAL_ADC_Stop(&adc);

  // 2. 按字节流示例的协议发送帧头、长度、两个大端字节和 XOR 校验。
  const uint8_t high = (uint8_t)(raw >> 8U);
  const uint8_t low = (uint8_t)raw;
  const uint8_t frame[] = {0xaaU, 2U, high, low, (uint8_t)(2U ^ high ^ low)};
  if (HAL_UART_Transmit(&uart, frame, sizeof(frame), 20U) == HAL_OK)
  {
    ++sent_frames;
  }
  else
  {
    ++serial_errors;
  }
}

int main(void)
{
  // 1. 先准备时基与外设，再执行周期采样。
  HAL_Init();
  setup_peripherals();
  for (;;)
  {
    // 2. 采样或发送失败都会返回此处，等待后开始下一轮。
    sample_and_send();
    HAL_Delay(250U);
  }
}
