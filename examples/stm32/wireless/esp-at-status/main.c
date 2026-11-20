#include "stm32f4xx_hal.h"
#include "at_reply.h"

enum { ESP_OK, ESP_TIMEOUT, ESP_UART_ERROR, ESP_REJECTED, ESP_BUSY, ESP_MISSING_STATE };
static UART_HandleTypeDef uart;
static volatile HAL_StatusTypeDef init_status;
static volatile unsigned result;
static volatile unsigned attempt;
static volatile int wifi_state = -1;

void SysTick_Handler(void) { HAL_IncTick(); }

static HAL_StatusTypeDef uart_init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();
  GPIO_InitTypeDef pins = {
    .Pin = GPIO_PIN_9 | GPIO_PIN_10, .Mode = GPIO_MODE_AF_PP,
    .Pull = GPIO_PULLUP, .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF7_USART1,
  };
  HAL_GPIO_Init(GPIOA, &pins);
  uart.Instance = USART1;
  uart.Init = (UART_InitTypeDef){
    .BaudRate = 115200U, .WordLength = UART_WORDLENGTH_8B,
    .StopBits = UART_STOPBITS_1, .Parity = UART_PARITY_NONE, .Mode = UART_MODE_TX_RX,
    .HwFlowCtl = UART_HWCONTROL_NONE, .OverSampling = UART_OVERSAMPLING_16,
  };
  return HAL_UART_Init(&uart);
}

// 命令带 CRLF。500 ms 是整条回复的期限，失败时调用者停止本轮演示。
static unsigned command(const char *text, AtParser *reply)
{
  // 1. 开始前清理前一次等待留下的 RX/ORE 状态。串口同时只允许这一条命令在途。
  __HAL_UART_CLEAR_OREFLAG(&uart);
  if (HAL_UART_Transmit(&uart, (const uint8_t *)text, (uint16_t)strlen(text), 30U) != HAL_OK)
  {
    return ESP_UART_ERROR;
  }

  // 2. 每次接收一个字节并使用余下的时限。回显和通知交给 at_feed 判断。
  const uint32_t started = HAL_GetTick();
  uint32_t age;
  while ((age = HAL_GetTick() - started) < 500U)
  {
    uint8_t byte;
    HAL_StatusTypeDef status = HAL_UART_Receive(&uart, &byte, 1U, 500U - age);
    if (status == HAL_TIMEOUT) { return ESP_TIMEOUT; }
    if (status != HAL_OK) { return ESP_UART_ERROR; }

    // 3. 完整终止行交还 main。中间行继续积累，其中查询值先保留在 reply。
    switch (at_feed(reply, byte))
    {
      case AT_OK: return ESP_OK;
      case AT_ERROR: return ESP_REJECTED;
      case AT_BUSY: return ESP_BUSY;
      case AT_MORE: break;
    }
  }
  return ESP_TIMEOUT;
}

int main(void)
{
  // 1. 先配置 UART，给外部 ESP-AT 模块留出启动时间。
  HAL_Init();
  init_status = uart_init();
  if (init_status != HAL_OK) { for (;;) {} }
  HAL_Delay(1500U);

  // 2. 用 AT 确认波特率和往返通路。失败时保留结果，等人工检查后复位。
  AtParser handshake = {.wifi_state = -1};
  result = command("AT\r\n", &handshake);
  if (result != ESP_OK) { for (;;) {} }

  for (;;)
  {
    // 3. 每轮只有一个查询。有效状态行和 OK 都收到后，才把结果交给调试器。
    AtParser query = {.wifi_state = -1};
    ++attempt;
    result = command("AT+CWSTATE?\r\n", &query);
    if (result == ESP_OK && query.wifi_state < 0) { result = ESP_MISSING_STATE; }
    if (result != ESP_OK) { for (;;) {} }
    wifi_state = query.wifi_state;
    HAL_Delay(1000U);
  }
}
