#include "demo.h"
#include "event_groups.h"

#define CONFIG_READY ((EventBits_t)(1U << 0))
#define DEVICE_READY ((EventBits_t)(1U << 1))
#define ALL_READY (CONFIG_READY | DEVICE_READY)

typedef struct
{
  EventBits_t bit;
  uint32_t delay_ms;
} InitWork;

static const InitWork services[] = {{CONFIG_READY, 100U}, {DEVICE_READY, 750U}};
static EventGroupHandle_t ready_group;
static volatile uint32_t timeouts;
static volatile EventBits_t last_timeout_missing;
static volatile BaseType_t application_ready;

static void initialize_service(void *argument)
{
  const InitWork *work = argument;

  // 1. 用延时模拟配置加载或器件探测。只有准备完成后才设置属于本任务的位。
  vTaskDelay(pdMS_TO_TICKS(work->delay_ms));
  xEventGroupSetBits(ready_group, work->bit);

  // 2. 就绪位保留给其他任务。本示例只初始化一次，因此任务此后挂起。
  for (;;)
  {
    vTaskSuspend(NULL);
  }
}

static void wait_for_services(void *argument)
{
  (void)argument;
  for (;;)
  {
    // 1. 等两位都置为 1。每 300 ms 检查一次，保留各服务已经设置的位。
    const EventBits_t bits = xEventGroupWaitBits(ready_group, ALL_READY, pdFALSE, pdTRUE,
                                                pdMS_TO_TICKS(300U));
    if ((bits & ALL_READY) == ALL_READY)
    {
      // 2. 两个初始化结果均已就绪。把就绪状态交给应用，用 LED 显示。
      application_ready = pdTRUE;
      HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
      break;
    }

    // 3. 任一位仍缺失就记录本轮超时和缺项。下一轮继续等已有服务之外的条件。
    last_timeout_missing = ALL_READY & ~bits;
    ++timeouts;
  }

  for (;;)
  {
    vTaskSuspend(NULL);
  }
}

int main(void)
{
  // 1. 先把事件组交给三个任务。创建失败时用 demo_fault 停下来。
  demo_init();
  ready_group = xEventGroupCreate();
  configASSERT(ready_group != NULL);
  configASSERT(xTaskCreate(initialize_service, "config", 256U, (void *)&services[0], 1U, NULL) == pdPASS);
  configASSERT(xTaskCreate(initialize_service, "device", 256U, (void *)&services[1], 1U, NULL) == pdPASS);
  configASSERT(xTaskCreate(wait_for_services, "startup", 256U, NULL, 2U, NULL) == pdPASS);

  // 2. 等待任务优先运行并阻塞，两个初始化任务随后各自按时报告进展。
  vTaskStartScheduler();
  demo_fault();
}
