#include "demo.h"
#include "queue.h"

enum { QUEUE_DEPTH = 2, STACK_WORDS = 256 };

typedef struct
{
  uint32_t sequence;
  uint32_t produced_at_ms;
} Packet;

static StaticQueue_t queue_control;
static uint8_t queue_storage[QUEUE_DEPTH * sizeof(Packet)];
static QueueHandle_t packet_queue;

static StaticTask_t producer_control, consumer_control, idle_control;
static StackType_t producer_stack[STACK_WORDS], consumer_stack[STACK_WORDS];
static StackType_t idle_stack[configMINIMAL_STACK_SIZE];

static volatile uint32_t sent, dropped, received, latest_sequence, latest_latency_ms;
static volatile UBaseType_t producer_min_free_words, consumer_min_free_words, max_queued;

// 调度器启动时向应用借用 Idle 任务的控制块和栈。空间由全局数组长期持有。
void vApplicationGetIdleTaskMemory(StaticTask_t **control, StackType_t **stack, uint32_t *words)
{
  *control = &idle_control;
  *stack = idle_stack;
  *words = configMINIMAL_STACK_SIZE;
}

static void produce(void *argument)
{
  (void)argument;
  uint32_t sequence = 0;
  TickType_t wake_at = xTaskGetTickCount();
  for (;;)
  {
    // 1. 每个周期创建一份消息。队列按值复制，成功后任务可以复用局部变量。
    const Packet packet = {.sequence = ++sequence, .produced_at_ms = HAL_GetTick()};
    if (xQueueSend(packet_queue, &packet, 0U) == pdPASS)
    {
      ++sent;
      const UBaseType_t waiting = uxQueueMessagesWaiting(packet_queue);
      if (waiting > max_queued)
      {
        max_queued = waiting;
      }
    }
    else
    {
      // 2. 队列满就丢掉这份新消息并计数，保持先前已入队的两条消息。
      ++dropped;
    }

    // 3. 记录本任务历史最小剩余栈字数，再回到固定周期。
    producer_min_free_words = uxTaskGetStackHighWaterMark(NULL);
    vTaskDelayUntil(&wake_at, pdMS_TO_TICKS(50U));
  }
}

static void consume(void *argument)
{
  (void)argument;
  for (;;)
  {
    // 1. 没消息就让出 CPU。成功后消费自己的副本，发布序号及等待时间。
    Packet packet;
    if (xQueueReceive(packet_queue, &packet, portMAX_DELAY) == pdPASS)
    {
      latest_sequence = packet.sequence;
      latest_latency_ms = HAL_GetTick() - packet.produced_at_ms;
      ++received;
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
    }

    // 2. 故意晚一些再收下一条，让生产者出现队列满分支。
    consumer_min_free_words = uxTaskGetStackHighWaterMark(NULL);
    vTaskDelay(pdMS_TO_TICKS(125U));
  }
}

int main(void)
{
  // 1. 队列与两项任务各自拿到全局控制块和存储。任何创建失败都停在启动阶段。
  demo_init();
  packet_queue = xQueueCreateStatic(QUEUE_DEPTH, sizeof(Packet), queue_storage, &queue_control);
  configASSERT(packet_queue != NULL);
  configASSERT(xTaskCreateStatic(produce, "produce", STACK_WORDS, NULL, 2U,
                                 producer_stack, &producer_control) != NULL);
  configASSERT(xTaskCreateStatic(consume, "consume", STACK_WORDS, NULL, 1U,
                                 consumer_stack, &consumer_control) != NULL);

  // 2. 调度器从应用取 Idle 的内存后开始交接。只有调度器返回时才触发故障停机。
  vTaskStartScheduler();
  demo_fault();
}
