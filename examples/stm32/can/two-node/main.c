#include <stdbool.h>
#include "stm32f4xx_hal.h"

#define REQUEST_ID 0x321U
#define REPLY_ID 0x322U

#if SKY_CAN_RESPONDER
#define RX_ID REQUEST_ID
#define TX_ID REPLY_ID
#else
#define RX_ID REPLY_ID
#define TX_ID REQUEST_ID
#endif

typedef enum { RX_NONE, RX_VALID, RX_FAILED } RxResult;

static CAN_HandleTypeDef can;
static volatile uint32_t init_stage, submitted, received, invalid_frames, submit_failures;
static volatile uint32_t reply_timeouts, stale_replies, last_esr;

void SysTick_Handler(void) { HAL_IncTick(); }

static HAL_StatusTypeDef can_init(void)
{
  // 1. 接收和发送引脚切到 AF9。收发器负责将逻辑信号转换到差分总线。
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_CAN1_CLK_ENABLE();
  GPIO_InitTypeDef pins = {
    .Pin = GPIO_PIN_8 | GPIO_PIN_9, .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL,
    .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF9_CAN1,
  };
  HAL_GPIO_Init(GPIOB, &pins);

  // 2. 默认 16 MHz APB1，每位 16 TQ，2 分频得到标称 500 kbit/s。
  can.Instance = CAN1;
  can.Init = (CAN_InitTypeDef){
    .Prescaler = 2U, .Mode = CAN_MODE_NORMAL, .SyncJumpWidth = CAN_SJW_1TQ,
    .TimeSeg1 = CAN_BS1_13TQ, .TimeSeg2 = CAN_BS2_2TQ,
    .AutoBusOff = ENABLE, .AutoRetransmission = DISABLE,
  };
  init_stage = 1U;
  if (HAL_CAN_Init(&can) != HAL_OK) { return HAL_ERROR; }

  // 3. 只把对应角色需要的标准数据帧交给 FIFO0。过滤器不处理总线 ACK。
  CAN_FilterTypeDef filter = {
    .FilterBank = 0U, .FilterMode = CAN_FILTERMODE_IDMASK, .FilterScale = CAN_FILTERSCALE_32BIT,
    .FilterIdHigh = RX_ID << 5U, .FilterMaskIdHigh = 0x7ffU << 5U,
    .FilterMaskIdLow = CAN_ID_EXT | CAN_RTR_REMOTE, .FilterFIFOAssignment = CAN_FILTER_FIFO0,
    .FilterActivation = ENABLE, .SlaveStartFilterBank = 14U,
  };
  init_stage = 2U;
  if (HAL_CAN_ConfigFilter(&can, &filter) != HAL_OK) { return HAL_ERROR; }
  init_stage = 3U;
  return HAL_CAN_Start(&can);
}

// 成功表示请求已提交到硬件邮箱。总线 ACK 和应用回包由后续观测验证。
static bool queue_frame(uint16_t sequence)
{
  CAN_TxHeaderTypeDef tx = {.StdId = TX_ID, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 2U};
  uint8_t data[8] = {(uint8_t)sequence, (uint8_t)(sequence >> 8)};
  uint32_t mailbox;
  if (HAL_CAN_AddTxMessage(&can, &tx, data, &mailbox) != HAL_OK)
  {
    ++submit_failures;
    return false;
  }
  ++submitted;
  return true;
}

// 只在 RX_VALID 时更新 sequence。空队列可立即返回，让调用者执行其他工作。
static RxResult take_frame(uint16_t *sequence)
{
  if (HAL_CAN_GetRxFifoFillLevel(&can, CAN_RX_FIFO0) == 0U)
  {
    return RX_NONE;
  }

  // 1. 从 FIFO 取一帧，验证应用协议的 ID、帧类型和负载长度。
  CAN_RxHeaderTypeDef rx;
  uint8_t data[8];
  if (HAL_CAN_GetRxMessage(&can, CAN_RX_FIFO0, &rx, data) != HAL_OK ||
      rx.IDE != CAN_ID_STD || rx.RTR != CAN_RTR_DATA || rx.StdId != RX_ID || rx.DLC != 2U)
  {
    ++invalid_frames;
    return RX_FAILED;
  }

  // 2. 低字节在前，序号用于应答方原样返回以及请求方配对。
  *sequence = (uint16_t)((uint16_t)data[0] | ((uint16_t)data[1] << 8));
  return RX_VALID;
}

#if !SKY_CAN_RESPONDER
static void request_once(uint16_t sequence)
{
  // 1. 占到发送邮箱后开始等应用回包。没有邮箱时，下一周期再试新请求。
  if (!queue_frame(sequence))
  {
    return;
  }

  // 2. 在 150 ms 内核对相同序号。旧的或其他序号不会结束本轮等待。
  const uint32_t start = HAL_GetTick();
  while ((uint32_t)(HAL_GetTick() - start) < 150U)
  {
    uint16_t reply;
    if (take_frame(&reply) == RX_VALID)
    {
      if (reply == sequence)
      {
        ++received;
        return;
      }
      ++stale_replies;
    }
    HAL_Delay(1U);
  }

  // 3. 超时返回 main，留下诊断计数。下轮仍可通信。
  ++reply_timeouts;
}
#endif

int main(void)
{
  // 1. 进入正常总线模式。初始化失败时保留阶段编号，停止收发。
  HAL_Init();
  if (can_init() != HAL_OK) { for (;;) {} }
  init_stage = 0U;

#if SKY_CAN_RESPONDER
  for (;;)
  {
    // 2. 应答方把刚接收的有效序号立即交给发送邮箱。无有效帧就继续轮询。
    uint16_t sequence;
    if (take_frame(&sequence) == RX_VALID)
    {
      ++received;
      queue_frame(sequence);
    }
    last_esr = CAN1->ESR;
    HAL_Delay(1U);
  }
#else
  uint16_t sequence = 0U;
  for (;;)
  {
    // 2. 请求方每轮用新序号发请求并等回包。记录 ESR 后留间隔给下轮通信。
    request_once(++sequence);
    last_esr = CAN1->ESR;
    HAL_Delay(750U);
  }
#endif
}
