#include "board.h"
#include "stm32f4xx_hal.h"
#include <stdbool.h>

static RxBlock pending;
static volatile bool has_block;
volatile uint32_t dropped_blocks;

void SysTick_Handler(void) { HAL_IncTick(); }

void app_rx_from_isr(const RxBlock *block)
{
  // 1. 中断收到新块时，若主循环还没取走上一块，就保留上一块，只给新丢的块计数。
  if (has_block)
  {
    ++dropped_blocks;
    return;
  }
  // 2. 先把数据复制到 pending，再通知主循环。回调返回后 DMA 就可以复用原缓冲区。
  pending = *block;
  has_block = true;
}

int main(void)
{
  // 1. 上电后先准备串口、DMA 和中断入口。初始化失败会停在 board_fault，无法进入接收循环。
  board_init();
  // 2. main 启动第一轮接收。此后每批数据收完，由接收回调启动下一轮。
  board_start_rx();
  for (;;)
  {
    // 3. 中断放好一块数据才会置位 has_block。槽为空时，主循环回到这里继续检查。
    if (has_block)
    {
      RxBlock local;
      // 4. 主循环暂时屏蔽中断，取走整块数据后清空标志。重新开放中断时，接收槽已经可用。
      __disable_irq();
      local = pending;
      has_block = false;
      __enable_irq();
      // 5. 串口回传的是 local 副本，这期间中断可以接收下一块。回传成功后再查槽，失败则停在故障码 5。
      board_echo(&local);
    }
  }
}
