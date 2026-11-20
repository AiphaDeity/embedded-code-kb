#include "demo.h"
#include <stdbool.h>

enum { CLOCK_HZ = 10000U, WORK_PERIOD_MS = 20U, SAMPLE_PERIOD_MS = 500U };

typedef struct
{
  uint32_t clock, light, heavy, idle;
} RunTimes;

static TaskHandle_t light_task, heavy_task;
static const uint32_t light_work = 10U, heavy_work = 30U;
static volatile RunTimes window;
static volatile uint32_t windows, snapshot_errors, clock_wraps;

void run_time_clock_init(void)
{
  // 1. 内核启动调度前调用。默认 HSI 与 APB1 分频为 1，TIM2 输入为 SystemCoreClock。
  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
  (void)RCC->APB1ENR;
  TIM2->PSC = SystemCoreClock / CLOCK_HZ - 1U;
  TIM2->ARR = UINT32_MAX;
  TIM2->EGR = TIM_EGR_UG;

  // 2. 装入预分频后自由运行，不使能中断。内核切换任务时直接读取 CNT。
  TIM2->CR1 = TIM_CR1_CEN;
}

static void load_task(void *argument)
{
  const uint32_t work_counts = *(const uint32_t *)argument;
  TickType_t wake = xTaskGetTickCount();
  for (;;)
  {
    // 1. 用一段有界的忙等模拟占用 CPU。计数单位为 100 us，暂停时间会包含抢占造成的等待。
    const uint32_t start = TIM2->CNT;
    while (TIM2->CNT - start < work_counts) {}

    // 2. 让出 CPU 到下一个周期。阻塞期间的时间记给实际运行的任务或 Idle。
    vTaskDelayUntil(&wake, pdMS_TO_TICKS(WORK_PERIOD_MS));
  }
}

// 成功时把同一次快照的自由运行计数、两个负载任务和 Idle 的累计计数交给调用者。
static bool collect(RunTimes *sample)
{
  TaskStatus_t tasks[4];
  uint32_t clock = 0;
  const UBaseType_t count = uxTaskGetSystemState(tasks, 4U, &clock);
  if (count != 4U) { return false; }

  const TaskHandle_t idle_task = xTaskGetIdleTaskHandle();
  *sample = (RunTimes){.clock = clock};
  for (UBaseType_t i = 0; i < count; ++i)
  {
    const TaskHandle_t task = tasks[i].xHandle;
    const uint32_t elapsed = tasks[i].ulRunTimeCounter;
    if (task == light_task) { sample->light = elapsed; }
    else if (task == heavy_task) { sample->heavy = elapsed; }
    else if (task == idle_task) { sample->idle = elapsed; }
  }
  return true;
}

static void monitor_task(void *argument)
{
  (void)argument;
  RunTimes previous = {0};
  bool have_previous = false;
  TickType_t wake = xTaskGetTickCount();
  for (;;)
  {
    // 1. 每 500 ms 取四个任务的快照。失败后重新取起点，避免跨故障窗口。
    RunTimes current;
    vTaskDelayUntil(&wake, pdMS_TO_TICKS(SAMPLE_PERIOD_MS));
    if (!collect(&current))
    {
      ++snapshot_errors;
      have_previous = false;
      continue;
    }

    // 2. 两次快照相减才得到本窗口的计数。检测到 TIM2 回绕时丢弃跨界窗口。
    if (have_previous && current.clock < previous.clock) { ++clock_wraps; }
    else if (have_previous)
    {
      window = (RunTimes){
        .clock = current.clock - previous.clock,
        .light = current.light - previous.light,
        .heavy = current.heavy - previous.heavy,
        .idle = current.idle - previous.idle,
      };
      ++windows;
    }
    previous = current;
    have_previous = true;
  }
}

int main(void)
{
  // 1. 创建两个同优先级负载任务。监测任务优先级高一档，每次采样后马上阻塞。
  demo_init();
  configASSERT(xTaskCreate(load_task, "light", 256U, (void *)&light_work, 1U, &light_task) == pdPASS);
  configASSERT(xTaskCreate(load_task, "heavy", 256U, (void *)&heavy_work, 1U, &heavy_task) == pdPASS);
  configASSERT(xTaskCreate(monitor_task, "monitor", 384U, NULL, 2U, NULL) == pdPASS);

  // 2. 内核先准备 Idle 和 TIM2，再进入最高优先级的监测任务建立采样周期。
  vTaskStartScheduler();
  demo_fault();
}
