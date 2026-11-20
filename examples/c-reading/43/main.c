#include "stm32f407xx.h"
#include "rx.h"

static volatile RxQueue queue;
static volatile Decoder decoder;
static volatile unsigned uart_errors;

// 独立固件从复位默认时钟启动，PCLK1 为 16 MHz。
static void uart_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;
    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->APB1ENR;
    GPIOA->MODER = (GPIOA->MODER & ~(3u << 6)) | (2u << 6);
    GPIOA->AFR[0] = (GPIOA->AFR[0] & ~(15u << 12)) | (7u << 12);
    GPIOA->PUPDR = (GPIOA->PUPDR & ~(3u << 6)) | (1u << 6);

    USART2->CR1 = 0;
    USART2->CR2 = 0;
    USART2->CR3 = 0;
    USART2->BRR = (16000000u + 115200u / 2u) / 115200u;
    USART2->CR1 = USART_CR1_UE | USART_CR1_RE | USART_CR1_RXNEIE;
    NVIC_EnableIRQ(USART2_IRQn);
}

void USART2_IRQHandler(void)
{
    // 1. 先查状态再读数据，读取顺序同时用于清除接收错误。
    uint32_t status = USART2->SR;
    const uint32_t errors = USART_SR_ORE | USART_SR_FE | USART_SR_NE | USART_SR_PE;
    if ((status & (errors | USART_SR_RXNE)) == 0u) return;
    uint8_t byte = (uint8_t)USART2->DR;
    // 2. 错误字节不入队，只记录缺口。下一次成功入队把缺口交给主循环。
    if ((status & errors) != 0u) {
        ++uart_errors;
        queue.gap_pending = true;
        return;
    }
    // 3. 把有效字节送入队列。队列满时记录丢弃，ISR 随即返回。
    rx_push(&queue, byte);
}

int main(void)
{
    // 1. 初始化 USART2 和 NVIC，之后由 USART2_IRQHandler() 往队列交付字节。
    uart_init();
    for (;;) {
        // 2. 暂停中断，原子地取走一个队列元素。恢复中断后仍可接收后续字节。
        RxByte byte;
        uint32_t mask = __get_PRIMASK();
        __disable_irq();
        bool ready = rx_pop(&queue, &byte);
        __set_PRIMASK(mask);
        // 3. 解码器只处理本地副本。缺口标记会让解码器丢弃上一段残帧。
        if (ready) decode(&decoder, byte);
    }
}
