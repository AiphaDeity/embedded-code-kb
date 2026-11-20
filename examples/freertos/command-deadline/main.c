#include "demo.h"
#include "queue.h"
#include <stdbool.h>

enum { COMMAND_BUDGET_MS = 80U };

typedef struct
{
  uint32_t id;
  uint16_t input;
} Command;

typedef struct
{
  uint32_t id;
  uint32_t value;
} Reply;

static QueueHandle_t commands, replies;
static volatile uint32_t accepted, timed_out, old_replies, late_replies;
static volatile uint32_t request_drops, reply_drops, work_completed;
static volatile Reply latest;

static bool await_reply(uint32_t id, TickType_t started, Reply *out)
{
  const TickType_t budget = pdMS_TO_TICKS(COMMAND_BUDGET_MS);
  for (;;)
  {
    // 1. 只等待原始截止时间前剩余的 tick。每次丢掉旧应答后仍沿用同一起点。
    const TickType_t elapsed = xTaskGetTickCount() - started;
    if (elapsed >= budget) { return false; }
    Reply candidate;
    if (xQueueReceive(replies, &candidate, budget - elapsed) != pdPASS) { return false; }

    // 2. 超过截止时间的应答不能完成这次命令。异号的旧应答也不写入输出。
    if (xTaskGetTickCount() - started >= budget)
    {
      ++late_replies;
      return false;
    }
    if (candidate.id != id)
    {
      ++old_replies;
      continue;
    }
    *out = candidate;
    return true;
  }
}

static void client(void *argument)
{
  (void)argument;
  uint32_t sequence = 0U;
  for (;;)
  {
    // 1. 创建命令时分配序号。队列满时拒绝这次提交，下一轮用新的序号。
    const Command request = {.id = ++sequence, .input = 42U};
    const TickType_t started = xTaskGetTickCount();
    if (xQueueSend(commands, &request, 0U) != pdPASS)
    {
      ++request_drops;
    }
    else
    {
      // 2. 成功提交后才等待。只用序号匹配且在期限内的应答更新最近结果。
      Reply reply;
      if (await_reply(request.id, started, &reply))
      {
        latest = reply;
        ++accepted;
      }
      else
      {
        ++timed_out;
      }
    }
    vTaskDelay(pdMS_TO_TICKS(20U));
  }
}

static void worker(void *argument)
{
  (void)argument;
  for (;;)
  {
    // 1. 有任务才工作。交替模拟 15 ms 正常耗时与 125 ms 慢操作。
    Command request;
    if (xQueueReceive(commands, &request, portMAX_DELAY) != pdPASS) { continue; }
    vTaskDelay(pdMS_TO_TICKS((request.id & 1U) ? 15U : 125U));

    // 2. 应答沿用请求序号。客户端超时后任务仍可能完成，所以不能省略序号。
    const Reply reply = {.id = request.id, .value = (uint32_t)request.input * 3U + 1U};
    ++work_completed;
    if (xQueueSend(replies, &reply, 0U) != pdPASS) { ++reply_drops; }
  }
}

int main(void)
{
  // 1. 请求和应答分别按值入队。客户端与工作任务就绪后启动调度。
  demo_init();
  commands = xQueueCreate(2U, sizeof(Command));
  replies = xQueueCreate(2U, sizeof(Reply));
  configASSERT(commands != NULL && replies != NULL);
  configASSERT(xTaskCreate(client, "client", 256U, NULL, 2U, NULL) == pdPASS);
  configASSERT(xTaskCreate(worker, "worker", 256U, NULL, 1U, NULL) == pdPASS);
  vTaskStartScheduler();
  demo_fault();
}
