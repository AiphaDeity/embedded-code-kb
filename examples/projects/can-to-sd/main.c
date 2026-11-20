#include "../../stm32/common/clock84.h"
#include "logger.h"
#include "sdio_disk.h"
#include "can_log.h"

_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "queue indexes must use lock-free atomic operations");

enum { BATCH_LIMIT = 8 };
static CAN_HandleTypeDef can;
static FrameQueue queue;
static FATFS file_system;
static volatile unsigned stage, frames_received, queue_drops, format_errors, lines_written, can_error;
static volatile FRESULT log_status;
void SysTick_Handler(void) { HAL_IncTick(); }
void CAN1_RX0_IRQHandler(void) { HAL_CAN_IRQHandler(&can); }

void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *handle)
{
  if (handle != &can) { return; }

  // 1. 最多取三个 FIFO 槽。ISR 只交接记录，SD 卡操作留给主循环。
  for (unsigned i = 0; i < 3U && HAL_CAN_GetRxFifoFillLevel(handle, CAN_RX_FIFO0) != 0U; ++i)
  {
    CAN_RxHeaderTypeDef header;
    CanRecord record = {.time_ms = HAL_GetTick()};
    if (HAL_CAN_GetRxMessage(handle, CAN_RX_FIFO0, &header, record.data) != HAL_OK)
    {
      can_error = HAL_CAN_GetError(handle);
      break;
    }
    record.extended = header.IDE == CAN_ID_EXT;
    record.remote = header.RTR == CAN_RTR_REMOTE;
    record.id = record.extended ? header.ExtId : header.StdId;
    record.length = (uint8_t)header.DLC;
    ++frames_received;

    // 2. 队列满时丢新帧并累加，保留已等候落盘的先前记录。
    if (!frame_push(&queue, record)) { ++queue_drops; }
  }
}

static bool start_can(void)
{
  // 1. 连接外置 CAN 收发器，配置静默监听，42 MHz / (6 × 14) 得到 500 kbit/s。
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_CAN1_CLK_ENABLE();
  GPIO_InitTypeDef pins = {
    .Pin = GPIO_PIN_8 | GPIO_PIN_9, .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL,
    .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF9_CAN1,
  };
  HAL_GPIO_Init(GPIOB, &pins);
  can.Instance = CAN1;
  can.Init = (CAN_InitTypeDef){
    .Prescaler = 6U, .Mode = CAN_MODE_SILENT, .SyncJumpWidth = CAN_SJW_1TQ,
    .TimeSeg1 = CAN_BS1_11TQ, .TimeSeg2 = CAN_BS2_2TQ,
  };
  if (HAL_CAN_Init(&can) != HAL_OK) { return false; }

  // 2. 打开标准与扩展帧过滤路径，接收回调从 FIFO0 取出帧。
  CAN_FilterTypeDef filter = {
    .FilterBank = 0U, .FilterMode = CAN_FILTERMODE_IDMASK, .FilterScale = CAN_FILTERSCALE_32BIT,
    .FilterFIFOAssignment = CAN_FILTER_FIFO0, .FilterActivation = ENABLE, .SlaveStartFilterBank = 14U,
  };
  if (HAL_CAN_ConfigFilter(&can, &filter) != HAL_OK || HAL_CAN_Start(&can) != HAL_OK)
  {
    return false;
  }
  HAL_NVIC_SetPriority(CAN1_RX0_IRQn, 6U, 0U);
  HAL_NVIC_EnableIRQ(CAN1_RX0_IRQn);
  return HAL_CAN_ActivateNotification(&can, CAN_IT_RX_FIFO0_MSG_PENDING) == HAL_OK;
}

int main(void)
{
  // 1. 84 MHz 配置同时提供 SDIO 时钟与 CAN 的 APB1 时钟。确认 CAN 已启动再接收。
  HAL_Init();
  stage = 1U;
  if (sky_clock84() != HAL_OK || !start_can()) { for (;;) { __WFI(); } }
  stage = 2U;
  uint32_t last_write = HAL_GetTick();

  for (;;)
  {
    // 2. 每 250 ms 从队列取最多 8 条。整理为一个批次后才访问文件系统。
    const uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - last_write) < 250U) { __WFI(); continue; }
    last_write = now;
    char batch[BATCH_LIMIT * MAX_CSV_LINE];
    size_t used = 0U;
    unsigned rows = 0U;
    CanRecord record;
    for (unsigned i = 0; i < BATCH_LIMIT && frame_pop(&queue, &record); ++i)
    {
      const size_t length = format_can_record(&record, batch + used, sizeof(batch) - used);
      if (length != 0U) { used += length; ++rows; }
      else { ++format_errors; }
    }
    if (rows == 0U) { continue; }

    // 3. 确认关闭成功后才累计落盘。错误时暂停采集，避免重发半写批次。
    log_status = append_named_log(&file_system, "0:/CANLOG.CSV", batch, (UINT)used);
    if (log_status != FR_OK)
    {
      stage = 3U;
      HAL_CAN_DeactivateNotification(&can, CAN_IT_RX_FIFO0_MSG_PENDING);
      for (;;) { __WFI(); }
    }
    lines_written += rows;
  }
}
