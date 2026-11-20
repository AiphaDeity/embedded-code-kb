#include "demo.h"
#include "timers.h"

static TaskHandle_t led_handle;
static volatile uint32_t fired, handled;

static void on_period(TimerHandle_t timer)
{
  (void)timer;
  // 1. 回调在服务任务里运行。只报告本次到期，把后续操作交给消费者。
  ++fired;
  xTaskNotifyGive(led_handle);
}

static void update_led(void *argument)
{
  (void)argument;
  for (;;)
  {
    // 1. 消费者在自己的上下文等待。每取到一批到期事件，更新一次 LED。
    handled += ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
  }
}

int main(void)
{
  // 1. 消费者句柄先就绪，然后创建自动重载定时器。
  demo_init();
  configASSERT(xTaskCreate(update_led, "led", 256U, NULL, 2U, &led_handle) == pdPASS);
  TimerHandle_t timer = xTimerCreate("period", pdMS_TO_TICKS(500U), pdTRUE, NULL, on_period);
  configASSERT(timer != NULL);

  // 2. 投递启动命令后开启调度，服务任务随后处理这条命令。
  configASSERT(xTimerStart(timer, 0U) == pdPASS);
  vTaskStartScheduler();
  demo_fault();
}
