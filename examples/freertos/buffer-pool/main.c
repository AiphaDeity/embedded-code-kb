#include "demo.h"
#include "queue.h"

enum { POOL_COUNT = 3, READY_DEPTH = 2, STACK_WORDS = 256, PAYLOAD_BYTES = 64 };

typedef struct
{
  uint32_t sequence;
  uint8_t payload[PAYLOAD_BYTES];
} Buffer;

static Buffer pool[POOL_COUNT];
static QueueHandle_t free_queue, ready_queue;
static StaticQueue_t free_control, ready_control;
static uint8_t free_storage[POOL_COUNT * sizeof(Buffer *)];
static uint8_t ready_storage[READY_DEPTH * sizeof(Buffer *)];

static StaticTask_t producer_control, consumer_control, idle_control;
static StackType_t producer_stack[STACK_WORDS], consumer_stack[STACK_WORDS];
static StackType_t idle_stack[configMINIMAL_STACK_SIZE];

static volatile uint32_t submitted, consumed, no_free_slot, ready_queue_full;
static volatile uint32_t invalid_payloads, last_sequence, recycle_errors;

void vApplicationGetIdleTaskMemory(StaticTask_t **control, StackType_t **stack, uint32_t *words)
{
  *control = &idle_control;
  *stack = idle_stack;
  *words = configMINIMAL_STACK_SIZE;
}

// 归还成功后，调用者立刻放弃访问。归还失败代表池的所有权约定已被破坏。
static void recycle(Buffer *buffer)
{
  if (xQueueSend(free_queue, &buffer, 0U) != pdPASS)
  {
    ++recycle_errors;
    demo_fault();
  }
}

static uint8_t sample_byte(uint32_t sequence, unsigned offset)
{
  return (uint8_t)(sequence + offset);
}

static void produce(void *argument)
{
  (void)argument;
  TickType_t wake_at = xTaskGetTickCount();
  uint32_t sequence = 0U;
  for (;;)
  {
    // 1. 每个周期借用一个空块。池空时丢掉这次采样，等下一轮再借。
    ++sequence;
    Buffer *buffer;
    if (xQueueReceive(free_queue, &buffer, 0U) != pdPASS)
    {
      ++no_free_slot;
    }
    else
    {
      // 2. 借到的块由生产者独占。写完后传递指针，把整个块交给就绪队列。
      buffer->sequence = sequence;
      for (unsigned i = 0U; i < PAYLOAD_BYTES; ++i)
      {
        buffer->payload[i] = sample_byte(sequence, i);
      }

      if (xQueueSend(ready_queue, &buffer, 0U) == pdPASS)
      {
        ++submitted;
      }
      else
      {
        // 3. 队列满时交接未发生，生产者将本块立即归还，保留原队列中的两条。
        ++ready_queue_full;
        recycle(buffer);
      }
    }

    // 4. 等到下一次周期起点。之后只有重新借到的块才可写。
    vTaskDelayUntil(&wake_at, pdMS_TO_TICKS(25U));
  }
}

static void consume(void *argument)
{
  (void)argument;
  for (;;)
  {
    // 1. 等就绪指针。取出后独占该块，可以跨任务切换持有它。
    Buffer *buffer;
    if (xQueueReceive(ready_queue, &buffer, portMAX_DELAY) != pdPASS)
    {
      continue;
    }
    vTaskDelay(pdMS_TO_TICKS(80U));

    // 2. 模拟耗时处理后检查整块数据，再更新可观察结果。
    const uint32_t sequence = buffer->sequence;
    for (unsigned i = 0U; i < PAYLOAD_BYTES; ++i)
    {
      if (buffer->payload[i] != sample_byte(sequence, i))
      {
        ++invalid_payloads;
        break;
      }
    }
    last_sequence = sequence;
    ++consumed;
    HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);

    // 3. 把处理完的块交回空闲队列。归还后生产者可能马上改写它。
    recycle(buffer);
    vTaskDelay(pdMS_TO_TICKS(80U));
  }
}

int main(void)
{
  // 1. 两条静态队列各自复制指针。调度前把三个独立的块交给空闲队列。
  demo_init();
  free_queue = xQueueCreateStatic(POOL_COUNT, sizeof(Buffer *), free_storage, &free_control);
  ready_queue = xQueueCreateStatic(READY_DEPTH, sizeof(Buffer *), ready_storage, &ready_control);
  configASSERT(free_queue != NULL && ready_queue != NULL);
  for (unsigned i = 0U; i < POOL_COUNT; ++i)
  {
    recycle(&pool[i]);
  }

  // 2. 生产者与消费者各持有自己的任务栈。调度开始后由两条队列完成交接。
  configASSERT(xTaskCreateStatic(produce, "produce", STACK_WORDS, NULL, 2U,
                                 producer_stack, &producer_control) != NULL);
  configASSERT(xTaskCreateStatic(consume, "consume", STACK_WORDS, NULL, 1U,
                                 consumer_stack, &consumer_control) != NULL);
  vTaskStartScheduler();
  demo_fault();
}
