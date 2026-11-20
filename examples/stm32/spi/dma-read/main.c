#include <stdbool.h>
#include "stm32f4xx_hal.h"

enum { HEADER_SIZE = 4, READ_BYTES = 16, TRANSFER_SIZE = HEADER_SIZE + READ_BYTES };
typedef enum { TRANSFER_IDLE, TRANSFER_BUSY, TRANSFER_OK, TRANSFER_ERROR } Transfer;
static SPI_HandleTypeDef spi;
static DMA_HandleTypeDef rx_dma, tx_dma;
static uint8_t tx[TRANSFER_SIZE], rx[TRANSFER_SIZE];
static volatile Transfer result;
static volatile uint32_t success_count, failure_count, last_error;
static volatile uint8_t flash_data[READ_BYTES];

void SysTick_Handler(void) { HAL_IncTick(); }
void SPI1_IRQHandler(void) { HAL_SPI_IRQHandler(&spi); }
void DMA2_Stream0_IRQHandler(void) { HAL_DMA_IRQHandler(&rx_dma); }
void DMA2_Stream3_IRQHandler(void) { HAL_DMA_IRQHandler(&tx_dma); }

void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *handle)
{
  if (handle == &spi && result == TRANSFER_BUSY)
  {
    // 1. HAL 已处理总线收尾。把已收齐的缓冲交给 main。
    result = TRANSFER_OK;
  }
}

void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *handle)
{
  if (handle == &spi && result == TRANSFER_BUSY)
  {
    // 1. 主循环负责撤销传输和片选。错误事件不发布缓冲内容。
    last_error = HAL_SPI_GetError(handle);
    result = TRANSFER_ERROR;
  }
}

static HAL_StatusTypeDef init_dma(void)
{
  __HAL_RCC_DMA2_CLK_ENABLE();
  rx_dma.Instance = DMA2_Stream0;
  rx_dma.Init = (DMA_InitTypeDef){
    .Channel = DMA_CHANNEL_3, .Direction = DMA_PERIPH_TO_MEMORY, .MemInc = DMA_MINC_ENABLE,
    .PeriphDataAlignment = DMA_PDATAALIGN_BYTE, .MemDataAlignment = DMA_MDATAALIGN_BYTE,
    .Mode = DMA_NORMAL, .Priority = DMA_PRIORITY_HIGH, .FIFOMode = DMA_FIFOMODE_DISABLE,
  };
  tx_dma.Instance = DMA2_Stream3;
  tx_dma.Init = rx_dma.Init;
  tx_dma.Init.Direction = DMA_MEMORY_TO_PERIPH;
  if (HAL_DMA_Init(&rx_dma) != HAL_OK || HAL_DMA_Init(&tx_dma) != HAL_OK)
  {
    return HAL_ERROR;
  }
  __HAL_LINKDMA(&spi, hdmarx, rx_dma);
  __HAL_LINKDMA(&spi, hdmatx, tx_dma);
  return HAL_OK;
}

static HAL_StatusTypeDef init_spi(void)
{
  // 1. PA4 在事务之外保持高电平，三个时钟与数据脚交给 SPI1。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_SPI1_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  GPIO_InitTypeDef pins = {.Pin = GPIO_PIN_4, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOA, &pins);
  pins = (GPIO_InitTypeDef){
    .Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, .Mode = GPIO_MODE_AF_PP,
    .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF5_SPI1,
  };
  HAL_GPIO_Init(GPIOA, &pins);
  spi.Instance = SPI1;
  spi.Init = (SPI_InitTypeDef){
    .Mode = SPI_MODE_MASTER, .Direction = SPI_DIRECTION_2LINES, .DataSize = SPI_DATASIZE_8BIT,
    .CLKPolarity = SPI_POLARITY_LOW, .CLKPhase = SPI_PHASE_1EDGE, .NSS = SPI_NSS_SOFT,
    .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16, .FirstBit = SPI_FIRSTBIT_MSB,
  };
  if (HAL_SPI_Init(&spi) != HAL_OK || init_dma() != HAL_OK)
  {
    return HAL_ERROR;
  }

  // 2. 收发各有一个 DMA Stream。让 SysTick 能在 HAL 中断等待期间继续计时。
  HAL_NVIC_SetPriority(SysTick_IRQn, 0U, 0U);
  HAL_NVIC_SetPriority(SPI1_IRQn, 6U, 0U);
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 6U, 0U);
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 6U, 0U);
  HAL_NVIC_EnableIRQ(SPI1_IRQn);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);
  return HAL_OK;
}

static bool read_address_zero(void)
{
  // 1. 片选有效期间发完整命令。0x03 后跟 3 字节地址和用来产生时钟的占位数据。
  result = TRANSFER_BUSY;
  last_error = 0U;
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
  if (HAL_SPI_TransmitReceive_DMA(&spi, tx, rx, TRANSFER_SIZE) == HAL_OK)
  {
    const uint32_t start = HAL_GetTick();
    while (result == TRANSFER_BUSY && (uint32_t)(HAL_GetTick() - start) < 20U)
    {
      __WFI();
    }
  }

  // 2. 到期也要收尾。若无法停止 SPI/DMA，就释放片选并停机，防止复用仍被占用的缓冲区。
  const bool completed = result == TRANSFER_OK;
  if (!completed)
  {
    result = TRANSFER_ERROR;
    last_error |= HAL_SPI_GetError(&spi);
    if (HAL_SPI_Abort(&spi) != HAL_OK)
    {
      HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
      for (;;) { __WFI(); }
    }
  }
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  result = TRANSFER_IDLE;
  return completed;
}

int main(void)
{
  // 1. 事务固定读取地址 0，地址和占位字节保持为 0。
  HAL_Init();
  if (init_spi() != HAL_OK) { for (;;) {} }
  tx[0] = 0x03U;

  for (;;)
  {
    // 2. 完成后才复制负载，跳过命令和地址对应的四个接收字节。
    if (read_address_zero())
    {
      for (unsigned i = 0; i < READ_BYTES; ++i)
      {
        flash_data[i] = rx[HEADER_SIZE + i];
      }
      ++success_count;
    }
    else
    {
      ++failure_count;
    }

    // 3. 本轮接收结果留给调试器。下一轮重新拉低片选。
    HAL_Delay(250U);
  }
}
