#include <stdbool.h>
#include "stm32f4xx.h"

#define BAUD_RATE 115200U
#define UART_RX_ERRORS (USART_SR_ORE | USART_SR_NE | USART_SR_FE | USART_SR_PE)

typedef enum { RX_OK, RX_TIMEOUT, RX_ERROR } ReceiveResult;

static volatile uint32_t milliseconds;
static volatile uint32_t timeouts, errors, tx_timeouts, echoed;

void SysTick_Handler(void)
{
  ++milliseconds;
}

static void usart1_init(void)
{
  // 1. 使能 GPIOA 和 USART1。各读回一次，确保访问时对应时钟已经打开。
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
  (void)RCC->AHB1ENR;
  RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
  (void)RCC->APB2ENR;

  // 2. 将 PA9 / PA10 切到 AF7。接收空闲时上拉，发送用推挽。
  const uint32_t af_mask = (0xFU << 4) | (0xFU << 8);
  const uint32_t mode_mask = (3U << 18) | (3U << 20);
  GPIOA->AFR[1] = (GPIOA->AFR[1] & ~af_mask) | (7U << 4) | (7U << 8);
  GPIOA->OTYPER &= ~((1U << 9) | (1U << 10));
  GPIOA->PUPDR = (GPIOA->PUPDR & ~mode_mask) | (1U << 20);
  GPIOA->MODER = (GPIOA->MODER & ~mode_mask) | (2U << 18) | (2U << 20);

  // 3. 默认 PCLK2 是 16 MHz。16 倍过采样时，BRR 写入四舍五入后的 PCLK2 / 波特率。
  USART1->CR1 = 0U;
  USART1->CR2 = 0U;
  USART1->CR3 = 0U;
  USART1->BRR = (SystemCoreClock + BAUD_RATE / 2U) / BAUD_RATE;
  USART1->CR1 = USART_CR1_UE | USART_CR1_TE | USART_CR1_RE;
}

// 最多等 100 ms。只有 RX_OK 时才把八位数据写入 output。
static ReceiveResult receive_byte(uint8_t *output)
{
  const uint32_t start = milliseconds;
  while ((uint32_t)(milliseconds - start) < 100U)
  {
    // 1. 先取 SR，再在有数据或出错时读 DR。这组读取也清除错误标志。
    const uint32_t status = USART1->SR;
    if ((status & (USART_SR_RXNE | UART_RX_ERRORS)) != 0U)
    {
      const uint32_t data = USART1->DR;

      // 2. 出错时丢掉本轮数据。正确时才把读出的字节交给 main。
      if ((status & UART_RX_ERRORS) != 0U)
      {
        return RX_ERROR;
      }
      *output = (uint8_t)data;
      return RX_OK;
    }
  }
  return RX_TIMEOUT;
}

// 在发送寄存器空出时写入字节。返回值表示是否写入，最终线路发送由 USART 完成。
static bool queue_tx_byte(uint8_t value)
{
  const uint32_t start = milliseconds;
  while ((USART1->SR & USART_SR_TXE) == 0U)
  {
    if ((uint32_t)(milliseconds - start) >= 20U)
    {
      return false;
    }
  }
  USART1->DR = value;
  return true;
}

int main(void)
{
  // 1. 复位默认使用 HSI 和总线 /1。建立毫秒时基，再让 USART1 接管引脚。
  if (SysTick_Config(SystemCoreClock / 1000U) != 0U)
  {
    for (;;) {}
  }
  usart1_init();

  for (;;)
  {
    // 2. 等最多 100 ms。超时和错误分别计数，然后开始下一轮接收。
    uint8_t byte;
    const ReceiveResult result = receive_byte(&byte);
    if (result == RX_TIMEOUT)
    {
      ++timeouts;
      continue;
    }
    if (result == RX_ERROR)
    {
      ++errors;
      continue;
    }

    // 3. 收到的字节进入发送寄存器。等待 TXE 超时时记一次失败，再回到接收。
    if (queue_tx_byte(byte))
    {
      ++echoed;
    }
    else
    {
      ++tx_timeouts;
    }
  }
}
