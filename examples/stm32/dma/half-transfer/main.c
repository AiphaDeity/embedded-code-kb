#include "stm32f4xx_hal.h"

enum { HALF_WORDS = 8U, WORDS = 2U * HALF_WORDS };
enum { COPY_PENDING, COPY_OK, INIT_FAILED, START_FAILED, TRANSFER_FAILED, TIMEOUT, MISMATCH };

static DMA_HandleTypeDef dma;
static uint32_t source[WORDS];
static volatile uint32_t destination[WORDS];
static volatile uint32_t first_ready, second_ready, transfer_error;
static volatile uint32_t result, verified_first, verified_second, batches_ok;

void SysTick_Handler(void) { HAL_IncTick(); }
void DMA2_Stream0_IRQHandler(void) { HAL_DMA_IRQHandler(&dma); }

static void half_ready(DMA_HandleTypeDef *stream)
{
  (void)stream;
  // 1. DMA 已写完前八个字。正常模式只向后走，将第一半交给主循环。
  __DMB();
  first_ready = 1U;
}

static void full_ready(DMA_HandleTypeDef *stream)
{
  (void)stream;
  // 1. DMA 已写完第二半，等待主循环验证完成后再启动下一轮。
  __DMB();
  second_ready = 1U;
}

static void copy_error(DMA_HandleTypeDef *stream)
{
  transfer_error = HAL_DMA_GetError(stream);
}

static HAL_StatusTypeDef init_dma(void)
{
  __HAL_RCC_DMA2_CLK_ENABLE();
  dma.Instance = DMA2_Stream0;
  dma.Init = (DMA_InitTypeDef){
    .Channel = DMA_CHANNEL_0, .Direction = DMA_MEMORY_TO_MEMORY,
    .PeriphInc = DMA_PINC_ENABLE, .MemInc = DMA_MINC_ENABLE,
    .PeriphDataAlignment = DMA_PDATAALIGN_WORD, .MemDataAlignment = DMA_MDATAALIGN_WORD,
    .Mode = DMA_NORMAL, .Priority = DMA_PRIORITY_LOW,
    .FIFOMode = DMA_FIFOMODE_ENABLE, .FIFOThreshold = DMA_FIFO_THRESHOLD_FULL,
    .MemBurst = DMA_MBURST_SINGLE, .PeriphBurst = DMA_PBURST_SINGLE,
  };
  if (HAL_DMA_Init(&dma) != HAL_OK) { return HAL_ERROR; }
  if (HAL_DMA_RegisterCallback(&dma, HAL_DMA_XFER_HALFCPLT_CB_ID, half_ready) != HAL_OK ||
      HAL_DMA_RegisterCallback(&dma, HAL_DMA_XFER_CPLT_CB_ID, full_ready) != HAL_OK ||
      HAL_DMA_RegisterCallback(&dma, HAL_DMA_XFER_ERROR_CB_ID, copy_error) != HAL_OK)
  {
    return HAL_ERROR;
  }
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 6U, 0U);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  return HAL_OK;
}

static uint32_t wait_and_compare(const volatile uint32_t *ready, unsigned offset, uint32_t started)
{
  // 1. 等本半区的事件，也检查整轮超时和 DMA 错误。未交付的半区不读取。
  while (!*ready && transfer_error == 0U)
  {
    if (HAL_GetTick() - started >= 100U) { return TIMEOUT; }
  }
  if (transfer_error != 0U) { return TRANSFER_FAILED; }

  // 2. 事件已发布，随后逐字比对。正常模式本轮不会再次改写这个半区。
  __DMB();
  for (unsigned i = offset; i < offset + HALF_WORDS; ++i)
  {
    if (destination[i] != source[i]) { return MISMATCH; }
  }
  return COPY_OK;
}

int main(void)
{
  // 1. 先注册半传输、全传输和错误回调，随后才能启动 DMA。
  HAL_Init();
  if (init_dma() != HAL_OK)
  {
    result = INIT_FAILED;
    for (;;) {}
  }

  for (;;)
  {
    // 2. 本轮源数据准备完成，再清事件并启动一次 16 字搬运。
    for (unsigned i = 0U; i < WORDS; ++i)
    {
      source[i] = 0x11220000U + batches_ok * WORDS + i;
      destination[i] = 0U;
    }
    first_ready = second_ready = transfer_error = 0U;
    result = COPY_PENDING;
    const uint32_t started = HAL_GetTick();
    __DMB();
    if (HAL_DMA_Start_IT(&dma, (uint32_t)(uintptr_t)source,
                         (uint32_t)(uintptr_t)destination, WORDS) != HAL_OK)
    {
      result = START_FAILED;
      for (;;) {}
    }

    // 3. 分别等待并验证前后两半。两段都交付后，主循环才接受这一轮。
    result = wait_and_compare(&first_ready, 0U, started);
    if (result == COPY_OK)
    {
      ++verified_first;
      result = wait_and_compare(&second_ready, HALF_WORDS, started);
    }
    if (result != COPY_OK)
    {
      // 4. 现场已经记录。若通道仍在搬运，先中止，然后停下来供调试。
      if (HAL_DMA_GetState(&dma) == HAL_DMA_STATE_BUSY) { HAL_DMA_Abort(&dma); }
      for (;;) {}
    }
    ++verified_second;
    ++batches_ok;
    HAL_Delay(500U);
  }
}
