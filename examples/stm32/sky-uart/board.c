#include "board.h"
#include "stm32f4xx_hal.h"

static UART_HandleTypeDef uart;
static DMA_HandleTypeDef rx_dma;
static RxBlock dma_block;
volatile uint32_t fault_code;
volatile uint32_t uart_error_count;

void board_fault(uint32_t code)
{
  __disable_irq();
  fault_code = code;
  for (;;) {}
}

void board_init(void)
{
  // 1. 上电复位后的内部 HSI 为 16 MHz，两个示例共享同一时钟和波特率。
  if (HAL_Init() != HAL_OK) { board_fault(1); }
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_DMA2_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();

  // 2. 把 PA9 和 PA10 接到 USART1，分别用于发送和接收。
  GPIO_InitTypeDef pins = {0};
  pins.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  pins.Mode = GPIO_MODE_AF_PP;
  pins.Pull = GPIO_PULLUP;
  pins.Speed = GPIO_SPEED_FREQ_HIGH;
  pins.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &pins);

  // 3. 按串口终端的 115200、8N1 配置 USART1。初始化失败停在故障码 2。
  uart.Instance = USART1;
  uart.Init.BaudRate = 115200;
  uart.Init.WordLength = UART_WORDLENGTH_8B;
  uart.Init.StopBits = UART_STOPBITS_1;
  uart.Init.Parity = UART_PARITY_NONE;
  uart.Init.Mode = UART_MODE_TX_RX;
  uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  uart.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&uart) != HAL_OK) { board_fault(2); }

  // 4. 配置 DMA 接收通道，再把它交给 UART 使用。DMA 初始化失败停在故障码 3。
  rx_dma.Instance = DMA2_Stream5;
  rx_dma.Init.Channel = DMA_CHANNEL_4;
  rx_dma.Init.Direction = DMA_PERIPH_TO_MEMORY;
  rx_dma.Init.PeriphInc = DMA_PINC_DISABLE;
  rx_dma.Init.MemInc = DMA_MINC_ENABLE;
  rx_dma.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
  rx_dma.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
  rx_dma.Init.Mode = DMA_NORMAL;
  rx_dma.Init.Priority = DMA_PRIORITY_MEDIUM;
  rx_dma.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
  if (HAL_DMA_Init(&rx_dma) != HAL_OK) { board_fault(3); }
  __HAL_LINKDMA(&uart, hdmarx, rx_dma);

  // 5. 两个入口使用相同的抢占优先级，交接同一 dma_block 时不会互相打断。
  HAL_NVIC_SetPriority(USART1_IRQn, 6, 0);
  HAL_NVIC_SetPriority(DMA2_Stream5_IRQn, 6, 0);
  HAL_NVIC_EnableIRQ(USART1_IRQn);
  HAL_NVIC_EnableIRQ(DMA2_Stream5_IRQn);
}

void board_start_rx(void)
{
  // 1. 把 dma_block.bytes 交给本轮 DMA。启动失败会保留故障码 4 并停止运行。
  if (HAL_UARTEx_ReceiveToIdle_DMA(&uart, dma_block.bytes, sizeof(dma_block.bytes)) != HAL_OK)
  {
    board_fault(4);
  }
}

void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&uart); }
void DMA2_Stream5_IRQHandler(void) { HAL_DMA_IRQHandler(&rx_dma); }

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *handle, uint16_t size)
{
  // 1. 这里只处理 USART1。DMA 搬到一半时继续等，同一批数据在空闲或缓冲区填满时交给应用。
  if (handle != &uart || HAL_UARTEx_GetRxEventType(handle) == HAL_UART_RXEVENT_HT)
  {
    return;
  }
  // 2. 普通模式每轮都从下标 0 开始。把本轮有效长度连同数据交给主循环的接收槽或任务队列。
  dma_block.length = size;
  app_rx_from_isr(&dma_block);
  // 3. 应用已复制数据，或已把槽满、队列满记为丢块。两条路径都要重启 DMA，才能接收下一批。
  board_start_rx();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *handle)
{
  if (handle == &uart)
  {
    // 1. 本轮接收失败，丢掉未交接的数据，只增加错误计数。
    ++uart_error_count;
    // 2. 尝试重启接收，让下一批从缓冲区开头写入。启动失败由 board_start_rx() 停机。
    board_start_rx();
  }
}

void board_echo(const RxBlock *block)
{
  if (HAL_UART_Transmit(&uart, block->bytes, block->length, 100) != HAL_OK)
  {
    board_fault(5);
  }
}
