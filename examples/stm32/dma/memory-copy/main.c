#include "stm32f4xx_hal.h"

static DMA_HandleTypeDef dma;
static uint32_t source[] = {0x11223344U, 0x22334455U, 0xAABBCCDDU, 0x01020304U};
static volatile uint32_t destination[sizeof(source) / sizeof(source[0])];
static volatile int result;
static volatile uint32_t dma_error;

void SysTick_Handler(void) { HAL_IncTick(); }

static int copy_and_check(void)
{
  // 1. DMA2 的内存到内存模式用 FIFO。源和目标都按 32 位字递增。
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
  if (HAL_DMA_Init(&dma) != HAL_OK) { return -1; }

  // 2. 交给 DMA 的长度以元素计。完成前不访问目标数组。
  const uint32_t words = sizeof(source) / sizeof(source[0]);
  if (HAL_DMA_Start(&dma, (uint32_t)(uintptr_t)source,
                    (uint32_t)(uintptr_t)destination, words) != HAL_OK) { return -2; }
  if (HAL_DMA_PollForTransfer(&dma, HAL_DMA_FULL_TRANSFER, 100U) != HAL_OK)
  {
    // 等候报错或超时，先保存 DMA 错误码，再尝试中止。本轮跳过目标数组比对。
    dma_error = HAL_DMA_GetError(&dma);
    HAL_DMA_Abort(&dma);
    return -3;
  }

  // 3. 搬运结束后 CPU 才逐字验证并发布结果。
  for (uint32_t i = 0; i < words; ++i)
  {
    if (source[i] != destination[i]) { return -4; }
  }
  return 1;
}

int main(void)
{
  // 1. 启用 HAL 时基，供 copy_and_check() 限制 DMA 等待时间。
  HAL_Init();
  // 2. 搬运完成才返回逐字比对结果。提前失败时返回负值，具体阶段看返回码。
  result = copy_and_check();
  // 3. 等待调试器查看 result 和 dma_error，主循环保留本轮现场。
  for (;;) {}
}
