#include "stm32f4xx_hal.h"

#define FRAME_ID 0x321U
static CAN_HandleTypeDef can;
static volatile uint32_t stage, loopback_ok, received_id;
static volatile uint8_t received_data[8];

void SysTick_Handler(void) { HAL_IncTick(); }

static void test_loopback(void)
{
  // 1. 准备静默回环和 500 kbit/s 的位时序。
  __HAL_RCC_CAN1_CLK_ENABLE();
  can.Instance = CAN1;
  can.Init = (CAN_InitTypeDef){
    .Prescaler = 2U, .Mode = CAN_MODE_SILENT_LOOPBACK, .SyncJumpWidth = CAN_SJW_1TQ,
    .TimeSeg1 = CAN_BS1_13TQ, .TimeSeg2 = CAN_BS2_2TQ,
  };
  stage = 1U;
  if (HAL_CAN_Init(&can) != HAL_OK) { return; }

  // 2. 32 位筛选字中标准 ID 在 bit31:21。低半字还检查 IDE 与 RTR。
  CAN_FilterTypeDef filter = {
    .FilterBank = 0U, .FilterMode = CAN_FILTERMODE_IDMASK, .FilterScale = CAN_FILTERSCALE_32BIT,
    .FilterIdHigh = FRAME_ID << 5U, .FilterMaskIdHigh = 0x7ffU << 5U,
    .FilterMaskIdLow = CAN_ID_EXT | CAN_RTR_REMOTE, .FilterFIFOAssignment = CAN_FILTER_FIFO0,
    .FilterActivation = ENABLE, .SlaveStartFilterBank = 14U,
  };
  stage = 2U;
  if (HAL_CAN_ConfigFilter(&can, &filter) != HAL_OK) { return; }
  stage = 3U;
  if (HAL_CAN_Start(&can) != HAL_OK) { return; }

  // 3. 发送一帧，再给接收 FIFO 一个有限等待窗口。
  CAN_TxHeaderTypeDef tx = {.StdId = FRAME_ID, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 3U};
  uint8_t data[8] = {0x11U, 0x22U, 0x33U};
  uint32_t mailbox;
  stage = 4U;
  if (HAL_CAN_AddTxMessage(&can, &tx, data, &mailbox) != HAL_OK) { return; }

  stage = 5U;
  const uint32_t start = HAL_GetTick();
  while (HAL_CAN_GetRxFifoFillLevel(&can, CAN_RX_FIFO0) == 0U)
  {
    // 等待超过 100 ms 时返回 main，保留 stage 5 和未通过状态。
    if (HAL_GetTick() - start >= 100U) { return; }
  }
  CAN_RxHeaderTypeDef rx;
  uint8_t reply[8];
  if (HAL_CAN_GetRxMessage(&can, CAN_RX_FIFO0, &rx, reply) != HAL_OK) { return; }

  // 4. 先保存观测数据，再报告逐字节校验结果。
  received_id = rx.StdId;
  for (uint32_t i = 0; i < 8U; ++i) { received_data[i] = reply[i]; }
  loopback_ok = rx.StdId == FRAME_ID && rx.IDE == CAN_ID_STD && rx.RTR == CAN_RTR_DATA &&
                rx.DLC == 3U && reply[0] == data[0] && reply[1] == data[1] && reply[2] == data[2];
  stage = 6U;
}

int main(void)
{
  // 1. 初始化 HAL 时基，供 test_loopback() 计算收帧期限。
  HAL_Init();
  // 2. 只做一轮回环。失败时 stage 留在出错步骤，成功时可查看 received_data 和 loopback_ok。
  test_loopback();
  // 3. 保留本轮结果供调试器检查，主循环在此等待。
  for (;;) {}
}
