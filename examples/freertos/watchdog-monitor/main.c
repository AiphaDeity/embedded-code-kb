#include "demo.h"
#include "event_groups.h"

enum { PROGRESS_A = 1U, PROGRESS_B = 2U, PROGRESS_ALL = 3U, WINDOW_MS = 400U };

typedef struct
{
  EventBits_t bit;
  TickType_t period;
  volatile uint32_t *completed;
} Worker;

static EventGroupHandle_t progress;
static IWDG_HandleTypeDef watchdog;
static volatile uint32_t reset_flags, a_completed, b_completed, feeds;
static volatile uint32_t inject_missing_b, monitor_latched, missing_tasks;
static volatile HAL_StatusTypeDef watchdog_status;
static const Worker workers[] = {
  {PROGRESS_A, pdMS_TO_TICKS(75U), &a_completed},
  {PROGRESS_B, pdMS_TO_TICKS(130U), &b_completed},
};

static void run_worker(void *argument)
{
  const Worker *worker = argument;
  TickType_t next = xTaskGetTickCount();
  for (;;)
  {
    // 1. 等自己的业务周期。PA0 或调试注入只阻止 B 产生本轮进展。
    vTaskDelayUntil(&next, worker->period);
    if (worker->bit == PROGRESS_B &&
        (inject_missing_b != 0U || HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET))
    {
      continue;
    }

    // 2. 模拟业务完成后设置对应位。重复报告仍属于当前 Monitor 窗口。
    ++*worker->completed;
    xEventGroupSetBits(progress, worker->bit);
  }
}

static void monitor(void *argument)
{
  (void)argument;
  TickType_t boundary = xTaskGetTickCount();
  for (;;)
  {
    // 1. 到固定边界才结算。一次调用取出清除前的位并清除它们，下一窗口从空状态开始。
    vTaskDelayUntil(&boundary, pdMS_TO_TICKS(WINDOW_MS));
    const EventBits_t completed = xEventGroupClearBits(progress, PROGRESS_ALL);
    missing_tasks = PROGRESS_ALL & ~completed;

    // 2. 两项进展都齐才刷新硬件。任何一项缺失或刷新失败都进入锁定状态。
    if (missing_tasks == 0U)
    {
      watchdog_status = HAL_IWDG_Refresh(&watchdog);
      if (watchdog_status == HAL_OK)
      {
        ++feeds;
        continue;
      }
    }
    monitor_latched = 1U;

    // 3. 停止监控任务的喂狗路径。业务任务可继续，独立看门狗负责超时复位。
    for (;;) { vTaskSuspend(NULL); }
  }
}

int main(void)
{
  // 1. 保留复位来源再清标志。若此前由 IWDG 复位，PB2 LED 保持亮起。
  reset_flags = RCC->CSR;
  __HAL_RCC_CLEAR_RESET_FLAGS();
  demo_init();
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2,
                    (reset_flags & RCC_CSR_IWDGRSTF) ? GPIO_PIN_SET : GPIO_PIN_RESET);
  __HAL_RCC_GPIOA_CLK_ENABLE();
  GPIO_InitTypeDef pin = {.Pin = GPIO_PIN_0, .Mode = GPIO_MODE_INPUT, .Pull = GPIO_NOPULL};
  HAL_GPIO_Init(GPIOA, &pin);

  // 2. 先创建事件组、两项业务和高优先级 Monitor，再启动硬件看门狗。
  progress = xEventGroupCreate();
  configASSERT(progress != NULL);
  configASSERT(xTaskCreate(run_worker, "work-a", 256U, (void *)&workers[0], 1U, NULL) == pdPASS);
  configASSERT(xTaskCreate(run_worker, "work-b", 256U, (void *)&workers[1], 1U, NULL) == pdPASS);
  configASSERT(xTaskCreate(monitor, "monitor", 256U, NULL, 2U, NULL) == pdPASS);
  watchdog.Instance = IWDG;
  watchdog.Init.Prescaler = IWDG_PRESCALER_32;
  watchdog.Init.Reload = 1999U;
  watchdog_status = HAL_IWDG_Init(&watchdog);
  if (watchdog_status != HAL_OK) { demo_fault(); }

  // 3. 调度启动后只有 Monitor 能刷新看门狗。调度器异常返回时停机并由 IWDG 处理。
  vTaskStartScheduler();
  demo_fault();
}
