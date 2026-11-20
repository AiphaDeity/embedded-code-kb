#ifndef SKY_UART_BOARD_H
#define SKY_UART_BOARD_H

#include <stdint.h>

typedef struct
{
  uint16_t length;
  uint8_t bytes[32];
} RxBlock;

// 初始化天空星 USART1 与 DMA。接收启动前应准备好消费缓冲区或队列。
void board_init(void);
void board_start_rx(void);
// 在主循环或任务中调用，回传 length 个字节。
void board_echo(const RxBlock *block);
// 应用实现。UART/DMA 中断调用，block 仅在此次调用期间归接收方读取。
void app_rx_from_isr(const RxBlock *block);
// 停在故障现场，调试器可观察 fault_code。
void board_fault(uint32_t code);

#endif
