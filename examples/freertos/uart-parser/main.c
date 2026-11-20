#include "demo.h"
#include "queue.h"
#include "../../c/stream-frame/decoder.h"
#include "../../stm32/common/sky-usart1.h"

enum { RX_QUEUE_DEPTH = 16, ACK_SYNC = 0xAC, RX_RETRY_MS = 10 };

typedef struct
{
  uint32_t position;
  uint8_t byte;
} RxItem;

static UART_HandleTypeDef uart;
static QueueHandle_t rx_queue;
static uint8_t rx_byte;
static uint32_t position;
static volatile bool restart_required;
static volatile uint32_t pause_parser, queue_full_drops, uart_errors, stream_gaps;
static volatile uint32_t accepted_frames, ack_failures;
static volatile uint8_t last_payload[MAX_PAYLOAD], last_length;

void USART1_IRQHandler(void) { HAL_UART_IRQHandler(&uart); }

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *handle)
{
  if (handle != &uart || HAL_UART_GetError(handle) != HAL_UART_ERROR_NONE)
  {
    return;
  }

  // 1. 每个收到的字节先领序号。队列满也推进序号，让解析任务识别丢失位置。
  const RxItem item = {.position = ++position, .byte = rx_byte};
  BaseType_t task_woken = pdFALSE;
  if (xQueueSendFromISR(rx_queue, &item, &task_woken) != pdPASS) { ++queue_full_drops; }

  // 2. 入队的是副本。中断复用接收槽，挂好下一次接收再请求任务切换。
  if (HAL_UART_Receive_IT(handle, &rx_byte, 1U) != HAL_OK)
  {
    ++position;
    restart_required = true;
  }
  portYIELD_FROM_ISR(task_woken);
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *handle)
{
  if (handle == &uart)
  {
    // 1. 为可能遗失的字节留一个序号。恢复交给任务，等 HAL 中断处理退出再执行。
    ++uart_errors;
    ++position;
    restart_required = true;
  }
}

static void recover_rx(void)
{
  if (!restart_required) { return; }

  // 1. 临界区屏蔽 USART1。当前接收无 DMA，取消挂起并清残留后才能重新接收。
  taskENTER_CRITICAL();
  if (HAL_UART_AbortReceive(&uart) == HAL_OK)
  {
    __HAL_UART_CLEAR_OREFLAG(&uart);
    restart_required = HAL_UART_Receive_IT(&uart, &rx_byte, 1U) != HAL_OK;
  }
  taskEXIT_CRITICAL();
}

static void parse_stream(void *argument)
{
  (void)argument;
  Decoder decoder = {0};
  uint32_t next_position = 1U;

  // 1. 调度开始后才挂第一字节并打开中断，使 FromISR 接口运行时队列和任务均已就绪。
  configASSERT(HAL_UART_Receive_IT(&uart, &rx_byte, 1U) == HAL_OK);
  HAL_NVIC_SetPriority(USART1_IRQn, 6U, 0U);
  HAL_NVIC_EnableIRQ(USART1_IRQn);

  for (;;)
  {
    // 2. 等新字节，同时保留定期恢复接收的机会。调试时可以暂停消费来制造积压。
    recover_rx();
    if (pause_parser != 0U)
    {
      vTaskDelay(pdMS_TO_TICKS(RX_RETRY_MS));
      continue;
    }
    RxItem item;
    if (xQueueReceive(rx_queue, &item, pdMS_TO_TICKS(RX_RETRY_MS)) != pdPASS) { continue; }

    // 3. 序号跳过时丢掉半帧，从本字节重新寻找帧头。加法按无符号数自然回绕。
    if (item.position != next_position)
    {
      decoder = (Decoder){0};
      ++stream_gaps;
    }
    next_position = item.position + 1U;
    if (!decode_byte(&decoder, item.byte)) { continue; }

    // 4. 完整帧通过校验后才发布负载。应答只携带标志与长度，发送有自己的期限。
    for (uint8_t i = 0U; i < decoder.length; ++i) { last_payload[i] = decoder.payload[i]; }
    last_length = decoder.length;
    ++accepted_frames;
    uint8_t ack[] = {ACK_SYNC, decoder.length};
    if (HAL_UART_Transmit(&uart, ack, sizeof(ack), 10U) != HAL_OK) { ++ack_failures; }
  }
}

int main(void)
{
  // 1. 用同一套板级串口参数。队列只复制 RxItem，解析状态留在任务栈上。
  demo_init();
  configASSERT(sky_usart1_init(&uart) == HAL_OK);
  rx_queue = xQueueCreate(RX_QUEUE_DEPTH, sizeof(RxItem));
  configASSERT(rx_queue != NULL);
  configASSERT(xTaskCreate(parse_stream, "uart-parser", 384U, NULL, 2U, NULL) == pdPASS);

  // 2. 启动调度，由解析任务开放 USART1。调度器异常返回时停住供调试。
  vTaskStartScheduler();
  demo_fault();
}
