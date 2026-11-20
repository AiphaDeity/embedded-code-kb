#include "demo.h"
#include "semphr.h"

typedef struct
{
  uint32_t total;
  uint32_t writes[2];
} SharedState;

typedef struct { uint32_t index, period_ms; } Writer;

static volatile SharedState shared;
static volatile uint32_t timeouts[2];
static SemaphoreHandle_t state_mutex;
static const Writer writers[] = {{0U, 75U}, {1U, 125U}};

static void write_state(void *argument)
{
  const Writer *writer = argument;
  for (;;)
  {
    // 1. 取得同一把锁后，才允许修改共享记录。
    if (xSemaphoreTake(state_mutex, pdMS_TO_TICKS(10U)) == pdTRUE)
    {
      ++shared.total;
      ++shared.writes[writer->index];
      // 2. 更新完成立即归还锁，下一次延时在锁外执行。
      xSemaphoreGive(state_mutex);
    }
    else
    {
      ++timeouts[writer->index];
    }
    vTaskDelay(pdMS_TO_TICKS(writer->period_ms));
  }
}

int main(void)
{
  // 1. 创建带优先级继承的互斥锁，然后创建两个任务。
  demo_init();
  state_mutex = xSemaphoreCreateMutex();
  configASSERT(state_mutex != NULL);
  configASSERT(xTaskCreate(write_state, "fast", 256U, (void *)&writers[0], 2U, NULL) == pdPASS);
  configASSERT(xTaskCreate(write_state, "slow", 256U, (void *)&writers[1], 1U, NULL) == pdPASS);
  vTaskStartScheduler();
  demo_fault();
}
