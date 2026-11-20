#include "demo.h"

static TaskHandle_t consumer_handle;
static volatile uint32_t notified, consumed;

static void producer(void *argument)
{
  (void)argument;
  // 1. 记录周期基准。每轮按上一目标时刻推进，而不是按任务完成时间推进。
  TickType_t previous = xTaskGetTickCount();
  for (;;)
  {
    vTaskDelayUntil(&previous, pdMS_TO_TICKS(250U));
    // 2. 更新观察计数，再发出通知。高优先级消费者可能马上运行。
    ++notified;
    xTaskNotifyGive(consumer_handle);
  }
}

static void consumer(void *argument)
{
  (void)argument;
  for (;;)
  {
    // 1. 无通知时阻塞。唤醒后一次取走所有已累计的通知。
    consumed += ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
  }
}

int main(void)
{
  // 1. 先准备消费者句柄，再启动生产任务和调度器。
  demo_init();
  configASSERT(xTaskCreate(consumer, "consumer", 256U, NULL, 2U, &consumer_handle) == pdPASS);
  configASSERT(xTaskCreate(producer, "producer", 256U, NULL, 1U, NULL) == pdPASS);
  vTaskStartScheduler();
  demo_fault();
}
