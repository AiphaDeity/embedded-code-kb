#include "board.h"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

static QueueHandle_t rx_queue;
volatile uint32_t dropped_blocks;
// 练习时在调试器设为 500，使消费者落后，从而观察队列满。
volatile uint32_t consumer_delay_ms;

void xPortSysTickHandler(void);
void SysTick_Handler(void)
{
  HAL_IncTick();
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
    xPortSysTickHandler();
  }
}

void app_rx_from_isr(const RxBlock *block)
{
  BaseType_t wake_task = pdFALSE;
  // 1. 中断把整块数据复制进队列，供 echo_task 稍后取走。队列满时保留已入队的数据，记下新丢的一块。
  if (xQueueSendFromISR(rx_queue, block, &wake_task) != pdPASS)
  {
    ++dropped_blocks;
  }
  // 2. 若入队唤醒了优先级更高的任务，就请求在退出中断后切换任务。
  portYIELD_FROM_ISR(wake_task);
}

static void echo_task(void *argument)
{
  (void)argument;
  RxBlock local;
  // 1. 调度器第一次运行 echo_task 时，队列已经就绪。此时开启 DMA，中断收到的第一块数据就有地方存。
  board_start_rx();
  for (;;)
  {
    // 2. 队列为空时任务在这里阻塞。中断送来数据后，任务取出一份副本，取到才往下回传。
    if (xQueueReceive(rx_queue, &local, portMAX_DELAY) == pdPASS)
    {
      // 3. 按本块的有效长度回传，失败会停在故障码 5。中断可以同时把下一块送入队列。
      board_echo(&local);
      // 4. 正常回传后立即取下一块。练习中调大延时，只让消费任务等一会，便能观察队列逐渐积满。
      const uint32_t delay = consumer_delay_ms;
      if (delay != 0U) { vTaskDelay(pdMS_TO_TICKS(delay)); }
    }
  }
}

int main(void)
{
  // 1. main 先准备串口和 DMA，接收要等队列和消费任务都就绪后才启动。
  board_init();
  // 2. 先给四块数据留出队列，再创建 echo_task。任一资源创建失败，都停在故障码 6。
  rx_queue = xQueueCreate(4, sizeof(RxBlock));
  if (rx_queue == NULL || xTaskCreate(echo_task, "echo", 256, NULL, 2, NULL) != pdPASS)
  {
    board_fault(6);
  }
  // 3. 从这里把执行权交给调度器，接下来的接收循环在 echo_task。若启动调用返回，记录故障码 7 并停机。
  vTaskStartScheduler();
  board_fault(7);
}
