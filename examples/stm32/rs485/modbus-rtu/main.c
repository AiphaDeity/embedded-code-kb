#include "stm32f4xx_hal.h"
#include "rtu.h"

#define DIR_PIN GPIO_PIN_8
#define UART_ERRORS (USART_SR_PE | USART_SR_NE | USART_SR_FE | USART_SR_ORE)

static UART_HandleTypeDef uart;
static RtuRx rx;
static volatile HAL_StatusTypeDef status;
static volatile uint32_t sent, received, discarded, response_timeouts, tx_errors;
static volatile uint16_t value0, value1;
static volatile uint8_t last_exception;

void SysTick_Handler(void) { HAL_IncTick(); }

static HAL_StatusTypeDef bus_init(void)
{
  // 1. PA8 控制驱动与接收，初始低。PA9 和 PA10 由 USART1 负责。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_TIM2_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOA, DIR_PIN, GPIO_PIN_RESET);
  GPIO_InitTypeDef pins = {.Pin = DIR_PIN, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOA, &pins);
  pins = (GPIO_InitTypeDef){
    .Pin = GPIO_PIN_9 | GPIO_PIN_10, .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_PULLUP,
    .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF7_USART1,
  };
  HAL_GPIO_Init(GPIOA, &pins);

  // 2. 8 数据位加偶校验在 F407 上配置为 9 位。TIM2 保持 1 微秒每计数。
  uart.Instance = USART1;
  uart.Init = (UART_InitTypeDef){
    .BaudRate = 9600U, .WordLength = UART_WORDLENGTH_9B, .StopBits = UART_STOPBITS_1,
    .Parity = UART_PARITY_EVEN, .Mode = UART_MODE_TX_RX,
    .HwFlowCtl = UART_HWCONTROL_NONE, .OverSampling = UART_OVERSAMPLING_16,
  };
  TIM2->PSC = 15U;
  TIM2->ARR = UINT32_MAX;
  TIM2->EGR = TIM_EGR_UG;
  TIM2->CR1 = TIM_CR1_CEN;
  rx.last_us = TIM2->CNT;
  rx.active = true;
  rx.current.bad = true;
  return HAL_UART_Init(&uart);
}

static bool poll_bus(RtuFrame *frame)
{
  // 1. 优先读已接收字符。SR 后读 DR 同时清错误，异常字节所在帧被丢弃。
  const uint32_t flags = USART1->SR;
  if ((flags & (USART_SR_RXNE | UART_ERRORS)) != 0U)
  {
    const uint8_t value = (uint8_t)USART1->DR;
    return rtu_receive(&rx, TIM2->CNT, value, (flags & UART_ERRORS) != 0U, frame);
  }
  return rtu_finish(&rx, TIM2->CNT, frame);
}

static bool send_frame(const uint8_t *data, uint16_t size)
{
  // 1. 阻塞发送正常结束时 TC 已置位，随即撤销 DE。发送失败也释放总线。
  HAL_GPIO_WritePin(GPIOA, DIR_PIN, GPIO_PIN_SET);
  status = HAL_UART_Transmit(&uart, data, size, 30U);
  HAL_GPIO_WritePin(GPIOA, DIR_PIN, GPIO_PIN_RESET);
  if (status != HAL_OK)
  {
    ++tx_errors;
    return false;
  }
  ++sent;
  return true;
}

int main(void)
{
  // 1. 两端复用同一套方向控制、8E1 和帧定界。角色由构建目标决定。
  HAL_Init();
  status = bus_init();
  if (status != HAL_OK) { for (;;) {} }

  RtuFrame frame;
#if RTU_SERVER
  const uint16_t holding[2] = {0x1234U, 0xABCDU};
  for (;;)
  {
    // 2. 服务端等待完整帧。有效请求生成数据或异常回复，其他帧只记录丢弃。
    if (poll_bus(&frame))
    {
      uint8_t response[9];
      const size_t length = build_server_reply(&frame, holding, response);
      if (length != 0U)
      {
        ++received;
        send_frame(response, (uint16_t)length);
      }
      else if (frame.count != 0U)
      {
        ++discarded;
      }
    }
  }
#else
  uint8_t request[8];
  build_read_request(request, 0U, 2U);
  uint32_t sent_at = HAL_GetTick() - 500U;
  bool awaiting = false;
  for (;;)
  {
    // 2. 客户端先结算应答期限。期限内有请求在途才消费匹配应答。
    const uint32_t elapsed = HAL_GetTick() - sent_at;
    if (awaiting && elapsed >= 150U)
    {
      ++response_timeouts;
      awaiting = false;
    }
    if (poll_bus(&frame) && frame.count != 0U)
    {
      uint16_t values[2];
      uint8_t exception;
      const ReplyResult result = awaiting ? read_client_reply(&frame, values, &exception) : REPLY_IGNORED;
      if (result == REPLY_VALUES)
      {
        value0 = values[0];
        value1 = values[1];
        ++received;
        awaiting = false;
      }
      else if (result == REPLY_EXCEPTION)
      {
        last_exception = exception;
        awaiting = false;
      }
      else
      {
        ++discarded;
      }
    }

    // 3. 有请求在途就继续等待。总线空闲并到达发送节拍后再发下一条。
    if (!awaiting && !rx.active && elapsed >= 500U)
    {
      awaiting = send_frame(request, sizeof(request));
      sent_at = HAL_GetTick();
    }
  }
#endif
}
