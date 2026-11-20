#include "stm32f4xx_hal.h"

_Alignas(4) static uint16_t wave[] = {
  2048U, 2430U, 2755U, 2972U, 3048U, 2972U, 2755U, 2430U,
  2048U, 1666U, 1341U, 1124U, 1048U, 1124U, 1341U, 1666U,
};
static DAC_HandleTypeDef dac;
static DMA_HandleTypeDef dma;
static TIM_HandleTypeDef timer;
static volatile uint32_t stage, error;

void SysTick_Handler(void) { HAL_IncTick(); }
void DMA1_Stream5_IRQHandler(void) { HAL_DMA_IRQHandler(&dma); }
void TIM6_DAC_IRQHandler(void) { HAL_DAC_IRQHandler(&dac); }

static HAL_StatusTypeDef start_wave(void)
{
  // 1. 配置 PA4 和循环 DMA。表放在普通 SRAM，供 DMA 按 16 位依次读取。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_DMA1_CLK_ENABLE();
  __HAL_RCC_DAC_CLK_ENABLE();
  __HAL_RCC_TIM6_CLK_ENABLE();
  GPIO_InitTypeDef pin = {.Pin = GPIO_PIN_4, .Mode = GPIO_MODE_ANALOG};
  HAL_GPIO_Init(GPIOA, &pin);
  dma.Instance = DMA1_Stream5;
  dma.Init = (DMA_InitTypeDef){
    .Channel = DMA_CHANNEL_7, .Direction = DMA_MEMORY_TO_PERIPH,
    .PeriphInc = DMA_PINC_DISABLE, .MemInc = DMA_MINC_ENABLE,
    .PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD, .MemDataAlignment = DMA_MDATAALIGN_HALFWORD,
    .Mode = DMA_CIRCULAR, .Priority = DMA_PRIORITY_HIGH, .FIFOMode = DMA_FIFOMODE_DISABLE,
  };
  stage = 1U;
  if (HAL_DMA_Init(&dma) != HAL_OK) { return HAL_ERROR; }

  // 2. TIM6 每 1000 个 16 MHz 时钟产生更新事件，交给 DAC 通道 1 作触发。
  stage = 2U;
  timer.Instance = TIM6;
  timer.Init = (TIM_Base_InitTypeDef){.Prescaler = 0U, .Period = 999U};
  if (HAL_TIM_Base_Init(&timer) != HAL_OK) { return HAL_ERROR; }
  TIM_MasterConfigTypeDef master = {.MasterOutputTrigger = TIM_TRGO_UPDATE};
  if (HAL_TIMEx_MasterConfigSynchronization(&timer, &master) != HAL_OK) { return HAL_ERROR; }

  // 3. 先装 DAC 和 DMA，保留错误 IRQ。初始化首个输出值后再启动定时器。
  stage = 3U;
  dac.Instance = DAC;
  __HAL_LINKDMA(&dac, DMA_Handle1, dma);
  if (HAL_DAC_Init(&dac) != HAL_OK) { return HAL_ERROR; }
  DAC_ChannelConfTypeDef channel = {.DAC_Trigger = DAC_TRIGGER_T6_TRGO, .DAC_OutputBuffer = DAC_OUTPUTBUFFER_ENABLE};
  if (HAL_DAC_ConfigChannel(&dac, &channel, DAC_CHANNEL_1) != HAL_OK ||
      HAL_DAC_SetValue(&dac, DAC_CHANNEL_1, DAC_ALIGN_12B_R, wave[15]) != HAL_OK)
  {
    return HAL_ERROR;
  }
  HAL_NVIC_EnableIRQ(DMA1_Stream5_IRQn);
  HAL_NVIC_EnableIRQ(TIM6_DAC_IRQn);
  if (HAL_DAC_Start_DMA(&dac, DAC_CHANNEL_1, (const uint32_t *)(uintptr_t)wave,
                        sizeof(wave) / sizeof(wave[0]), DAC_ALIGN_12B_R) != HAL_OK)
  {
    return HAL_ERROR;
  }
  __HAL_DMA_DISABLE_IT(&dma, DMA_IT_HT | DMA_IT_TC);
  stage = 4U;
  return HAL_TIM_Base_Start(&timer);
}

void HAL_DAC_ErrorCallbackCh1(DAC_HandleTypeDef *handle)
{
  if (handle == &dac)
  {
    // 1. IRQ 中先停触发，再由主循环终止 DMA。
    __HAL_TIM_DISABLE(&timer);
    error = HAL_DAC_GetError(handle);
  }
}

void HAL_DAC_DMAUnderrunCallbackCh1(DAC_HandleTypeDef *handle)
{
  HAL_DAC_ErrorCallbackCh1(handle);
}

int main(void)
{
  // 1. 启动后定时器、DMA 和 DAC 自行生成波形。
  HAL_Init();
  if (start_wave() != HAL_OK) { for (;;) {} }
  stage = 0U;

  for (;;)
  {
    // 2. DMA 出错或来不及供数时停止输出流程，保留错误码等待调试。
    if (error != 0U)
    {
      HAL_DAC_Stop_DMA(&dac, DAC_CHANNEL_1);
      for (;;) { __WFI(); }
    }
    __WFI();
  }
}
