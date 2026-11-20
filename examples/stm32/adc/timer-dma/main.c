#include <stdbool.h>
#include "stm32f4xx_hal.h"

enum { HALF_SAMPLES = 8, TOTAL_SAMPLES = HALF_SAMPLES * 2U };

static ADC_HandleTypeDef adc;
static DMA_HandleTypeDef dma;
static TIM_HandleTypeDef trigger;
_Alignas(4) static volatile uint16_t samples[TOTAL_SAMPLES];
static volatile uint32_t average_raw, completed_halves, skipped_halves, setup_stage, adc_error;

void SysTick_Handler(void) { HAL_IncTick(); }
void DMA2_Stream0_IRQHandler(void) { HAL_DMA_IRQHandler(&dma); }
void ADC_IRQHandler(void) { HAL_ADC_IRQHandler(&adc); }

// offset 是 0 或 HALF_SAMPLES。DMA 写另一半时才允许处理本段。
static bool half_is_stable(unsigned offset)
{
  const uint32_t remaining = __HAL_DMA_GET_COUNTER(&dma);
  if (offset == 0U)
  {
    return remaining != 0U && remaining <= HALF_SAMPLES;
  }
  return remaining > HALF_SAMPLES && remaining <= TOTAL_SAMPLES;
}

static void publish_half(unsigned offset)
{
  // 1. 从 NDTR 判断当前写入区。回调进得太晚时跳过该半区。
  if (!half_is_stable(offset))
  {
    ++skipped_halves;
    return;
  }

  // 2. 只读已完成的 8 个采样。在发布前再检查一次 DMA 没有返回这个半区。
  uint32_t sum = 0U;
  for (unsigned i = 0; i < HALF_SAMPLES; ++i)
  {
    sum += samples[offset + i];
  }
  if (!half_is_stable(offset))
  {
    ++skipped_halves;
    return;
  }

  // 3. 读完就只发布平均值。调试器读取该值，不持有 DMA 数组。
  average_raw = sum / HALF_SAMPLES;
  ++completed_halves;
}

void HAL_ADC_ConvHalfCpltCallback(ADC_HandleTypeDef *handle)
{
  if (handle == &adc)
  {
    // 1. DMA 写到中点时，CPU 处理前八个采样。
    publish_half(0U);
  }
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *handle)
{
  if (handle == &adc)
  {
    // 1. DMA 写到末尾并回到开头时，CPU 处理后八个采样。
    publish_half(HALF_SAMPLES);
  }
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *handle)
{
  if (handle == &adc)
  {
    // 1. ADC/DMA 失败后关闭触发源。保留错误码供调试，通过复位重新开始。
    __HAL_TIM_DISABLE(&trigger);
    adc_error = HAL_ADC_GetError(handle);
  }
}

static HAL_StatusTypeDef setup_dma(void)
{
  __HAL_RCC_DMA2_CLK_ENABLE();
  dma.Instance = DMA2_Stream0;
  dma.Init = (DMA_InitTypeDef){
    .Channel = DMA_CHANNEL_0, .Direction = DMA_PERIPH_TO_MEMORY,
    .PeriphInc = DMA_PINC_DISABLE, .MemInc = DMA_MINC_ENABLE,
    .PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD, .MemDataAlignment = DMA_MDATAALIGN_HALFWORD,
    .Mode = DMA_CIRCULAR, .Priority = DMA_PRIORITY_MEDIUM, .FIFOMode = DMA_FIFOMODE_DISABLE,
  };
  return HAL_DMA_Init(&dma);
}

static HAL_StatusTypeDef setup_adc(void)
{
  // 1. PC0 进入模拟模式，ADC1 的通道 10 接手采样。
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_ADC1_CLK_ENABLE();
  GPIO_InitTypeDef pin = {.Pin = GPIO_PIN_0, .Mode = GPIO_MODE_ANALOG};
  HAL_GPIO_Init(GPIOC, &pin);

  // 2. 每次 TIM2 TRGO 只转换一次。持续发送 DMA 请求，让同一段缓冲反复填入。
  adc.Instance = ADC1;
  adc.Init = (ADC_InitTypeDef){
    .ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4, .Resolution = ADC_RESOLUTION_12B,
    .ScanConvMode = DISABLE, .ContinuousConvMode = DISABLE,
    .ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_RISING, .ExternalTrigConv = ADC_EXTERNALTRIGCONV_T2_TRGO,
    .DataAlign = ADC_DATAALIGN_RIGHT, .NbrOfConversion = 1U,
    .DMAContinuousRequests = ENABLE, .EOCSelection = ADC_EOC_SINGLE_CONV,
  };
  __HAL_LINKDMA(&adc, DMA_Handle, dma);
  if (HAL_ADC_Init(&adc) != HAL_OK) { return HAL_ERROR; }
  ADC_ChannelConfTypeDef channel = {
    .Channel = ADC_CHANNEL_10, .Rank = 1U, .SamplingTime = ADC_SAMPLETIME_144CYCLES,
  };
  return HAL_ADC_ConfigChannel(&adc, &channel);
}

static HAL_StatusTypeDef setup_trigger(void)
{
  __HAL_RCC_TIM2_CLK_ENABLE();
  __HAL_DBGMCU_FREEZE_TIM2();
  trigger.Instance = TIM2;
  trigger.Init = (TIM_Base_InitTypeDef){
    .Prescaler = 159U, .CounterMode = TIM_COUNTERMODE_UP, .Period = 999U,
    .ClockDivision = TIM_CLOCKDIVISION_DIV1,
  };
  if (HAL_TIM_Base_Init(&trigger) != HAL_OK) { return HAL_ERROR; }
  TIM_MasterConfigTypeDef master = {.MasterOutputTrigger = TIM_TRGO_UPDATE};
  return HAL_TIMEx_MasterConfigSynchronization(&trigger, &master);
}

int main(void)
{
  // 1. 定时器先配置但不启动，DMA 和 ADC 接好后再打开中断。
  HAL_Init();
  setup_stage = 1U;
  if (setup_trigger() != HAL_OK) { for (;;) {} }
  setup_stage = 2U;
  if (setup_dma() != HAL_OK) { for (;;) {} }
  setup_stage = 3U;
  if (setup_adc() != HAL_OK) { for (;;) {} }
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 6U, 0U);
  HAL_NVIC_SetPriority(ADC_IRQn, 6U, 0U);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  HAL_NVIC_EnableIRQ(ADC_IRQn);

  // 2. ADC 和接收缓冲区准备完成后，才允许 TIM2 每 10 ms 触发一次。
  setup_stage = 4U;
  if (HAL_ADC_Start_DMA(&adc, (uint32_t *)(uintptr_t)samples, TOTAL_SAMPLES) != HAL_OK) { for (;;) {} }
  setup_stage = 5U;
  if (HAL_TIM_Base_Start(&trigger) != HAL_OK)
  {
    HAL_ADC_Stop_DMA(&adc);
    for (;;) {}
  }
  setup_stage = 0U;

  for (;;)
  {
    // 3. 主循环不占用正在写入的缓冲区，完成和错误都由 IRQ 回调报告。
    __WFI();
  }
}
