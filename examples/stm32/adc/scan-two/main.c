#include "stm32f4xx_hal.h"

enum { SAMPLE_WAIT_MS = 20 };
typedef enum { SCAN_IDLE, SCAN_ACTIVE, SCAN_COMPLETE, SCAN_FAILED } ScanState;
static ADC_HandleTypeDef adc;
static DMA_HandleTypeDef dma;
_Alignas(4) static volatile uint16_t samples[2];
static volatile ScanState state;
static volatile uint32_t input_pc0, input_pc1, scans, failures, adc_error;

void SysTick_Handler(void) { HAL_IncTick(); }
void DMA2_Stream0_IRQHandler(void) { HAL_DMA_IRQHandler(&dma); }
void ADC_IRQHandler(void) { HAL_ADC_IRQHandler(&adc); }

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *handle)
{
  if (handle == &adc && state == SCAN_ACTIVE)
  {
    // 1. 两个位置都已搬完，main 可以一次发布一组数据。
    state = SCAN_COMPLETE;
  }
}

void HAL_ADC_ErrorCallback(ADC_HandleTypeDef *handle)
{
  if (handle == &adc && state == SCAN_ACTIVE)
  {
    // 1. 禁止 main 使用这组可能没收齐的数据。
    adc_error = HAL_ADC_GetError(handle);
    state = SCAN_FAILED;
  }
}

static HAL_StatusTypeDef init_scan(void)
{
  // 1. PC0 和 PC1 接模拟输入。DMA 普通模式每轮写两个半字。
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_ADC1_CLK_ENABLE();
  GPIO_InitTypeDef pins = {.Pin = GPIO_PIN_0 | GPIO_PIN_1, .Mode = GPIO_MODE_ANALOG};
  HAL_GPIO_Init(GPIOC, &pins);
  dma.Instance = DMA2_Stream0;
  dma.Init = (DMA_InitTypeDef){
    .Channel = DMA_CHANNEL_0, .Direction = DMA_PERIPH_TO_MEMORY, .MemInc = DMA_MINC_ENABLE,
    .PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD, .MemDataAlignment = DMA_MDATAALIGN_HALFWORD,
    .Mode = DMA_NORMAL, .FIFOMode = DMA_FIFOMODE_DISABLE,
  };
  if (HAL_DMA_Init(&dma) != HAL_OK) { return HAL_ERROR; }
  adc.Instance = ADC1;
  __HAL_LINKDMA(&adc, DMA_Handle, dma);

  // 2. 一次软件启动扫描两个 Rank，按规则序列顺序把结果交给 DMA。
  adc.Init = (ADC_InitTypeDef){
    .ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4, .Resolution = ADC_RESOLUTION_12B,
    .ScanConvMode = ENABLE, .ContinuousConvMode = DISABLE, .ExternalTrigConv = ADC_SOFTWARE_START,
    .DataAlign = ADC_DATAALIGN_RIGHT, .NbrOfConversion = 2U, .EOCSelection = ADC_EOC_SEQ_CONV,
  };
  if (HAL_ADC_Init(&adc) != HAL_OK) { return HAL_ERROR; }
  ADC_ChannelConfTypeDef channel = {
    .Channel = ADC_CHANNEL_10, .Rank = 1U, .SamplingTime = ADC_SAMPLETIME_144CYCLES,
  };
  if (HAL_ADC_ConfigChannel(&adc, &channel) != HAL_OK) { return HAL_ERROR; }
  channel.Channel = ADC_CHANNEL_11;
  channel.Rank = 2U;
  if (HAL_ADC_ConfigChannel(&adc, &channel) != HAL_OK) { return HAL_ERROR; }
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 6U, 0U);
  HAL_NVIC_SetPriority(ADC_IRQn, 6U, 0U);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  HAL_NVIC_EnableIRQ(ADC_IRQn);
  return HAL_OK;
}

int main(void)
{
  // 1. 配置扫描次序、半字缓冲与 IRQ，再启动第一轮。
  HAL_Init();
  if (init_scan() != HAL_OK) { for (;;) {} }
  for (;;)
  {
    // 2. 等待整组完成。起动失败、超时和 ADC 错误都不发布新数据。
    state = SCAN_ACTIVE;
    if (HAL_ADC_Start_DMA(&adc, (uint32_t *)(uintptr_t)samples, 2U) == HAL_OK)
    {
      const uint32_t started = HAL_GetTick();
      while (state == SCAN_ACTIVE && (uint32_t)(HAL_GetTick() - started) < SAMPLE_WAIT_MS)
      {
        __WFI();
      }
    }

    // 3. 关闭本轮后再复用缓冲。停止失败时保留故障并停机。
    const ScanState finished = state;
    state = SCAN_IDLE;
    if (HAL_ADC_Stop_DMA(&adc) != HAL_OK) { ++failures; for (;;) {} }
    if (finished == SCAN_COMPLETE)
    {
      input_pc0 = samples[0];
      input_pc1 = samples[1];
      ++scans;
    }
    else
    {
      ++failures;
    }
    HAL_Delay(100U);
  }
}
