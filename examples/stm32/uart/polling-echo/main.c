#include "stm32f4xx_hal.h"

#define ECHO_SIZE 1U
#define RX_TIMEOUT_MS 100U
#define TX_TIMEOUT_MS 20U

static UART_HandleTypeDef uart;

// 编译后可在调试器中观察这些统计值。
static volatile HAL_StatusTypeDef uart_init_status;
static volatile uint32_t rx_timeouts;
static volatile uint32_t rx_errors;
static volatile uint32_t tx_failures;
static volatile uint32_t echoed_blocks;

// 将天空星调试排针的 USART1 配成 115200 8N1。
static HAL_StatusTypeDef usart1_init(void)
{
  // 1. GPIOA 和 USART1 在不同的总线上。先分别使能，再配置引脚与控制器。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();

  // 2. 将 PA9 和 PA10 切到 AF7，由 USART1 接管发送和接收引脚。
  GPIO_InitTypeDef pins = {0};
  pins.Pin = GPIO_PIN_9 | GPIO_PIN_10;
  pins.Mode = GPIO_MODE_AF_PP;
  pins.Pull = GPIO_PULLUP;
  pins.Speed = GPIO_SPEED_FREQ_HIGH;
  pins.Alternate = GPIO_AF7_USART1;
  HAL_GPIO_Init(GPIOA, &pins);

  // 3. 配好串口两端约定的参数，把初始化结果交给 main 决定是否进入收发循环。
  uart.Instance = USART1;
  uart.Init.BaudRate = 115200U;
  uart.Init.WordLength = UART_WORDLENGTH_8B;
  uart.Init.StopBits = UART_STOPBITS_1;
  uart.Init.Parity = UART_PARITY_NONE;
  uart.Init.Mode = UART_MODE_TX_RX;
  uart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  uart.Init.OverSampling = UART_OVERSAMPLING_16;
  return HAL_UART_Init(&uart);
}

// 默认 HAL 毫秒时基用于计算收发超时。
void SysTick_Handler(void)
{
  HAL_IncTick();
}

int main(void)
{
  // 1. 先初始化 HAL 时基和串口。USART1 初始化失败时保留返回值，停在此处供调试。
  HAL_Init();
  uart_init_status = usart1_init();
  if (uart_init_status != HAL_OK)
  {
    for (;;)
    {
    }
  }

  uint8_t data[ECHO_SIZE];
  for (;;)
  {
    // 2. 主循环等待 HAL 收齐本轮数据。只有 HAL_OK 才把缓冲区交给发送步骤。
    const HAL_StatusTypeDef rx_status = HAL_UART_Receive(&uart, data, sizeof(data), RX_TIMEOUT_MS);
    if (rx_status != HAL_OK)
    {
      // 3. 普通等待超时单独计数。HAL 标记了接收错误或返回其他失败时记录错误，都回到第 2 步。
      if (rx_status == HAL_TIMEOUT && HAL_UART_GetError(&uart) == HAL_UART_ERROR_NONE)
      {
        ++rx_timeouts;
      }
      else
      {
        ++rx_errors;
      }
      continue;
    }

    // 4. 原样回显本轮收到的数据。发送未成功时记录失败，随后继续接收新的数据。
    if (HAL_UART_Transmit(&uart, data, sizeof(data), TX_TIMEOUT_MS) == HAL_OK)
    {
      ++echoed_blocks;
    }
    else
    {
      ++tx_failures;
    }
  }
}
