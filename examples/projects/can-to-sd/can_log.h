#ifndef KB_CAN_LOG_H
#define KB_CAN_LOG_H

#include <stdbool.h>
#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

enum { FRAME_QUEUE_SLOTS = 16, MAX_CSV_LINE = 56 };
typedef struct
{
  uint32_t time_ms, id;
  bool extended, remote;
  uint8_t length, data[8];
} CanRecord;

typedef struct
{
  CanRecord slots[FRAME_QUEUE_SLOTS];
  atomic_uint head, tail;
} FrameQueue;

// 单一 ISR 生产，单一主循环消费。容量为 15，满时新记录由调用者计数。
static inline bool frame_push(FrameQueue *queue, CanRecord record)
{
  const unsigned head = atomic_load_explicit(&queue->head, memory_order_relaxed);
  const unsigned next = (head + 1U) % FRAME_QUEUE_SLOTS;
  if (next == atomic_load_explicit(&queue->tail, memory_order_acquire))
  {
    return false;
  }
  queue->slots[head] = record;
  atomic_store_explicit(&queue->head, next, memory_order_release);
  return true;
}

static inline bool frame_pop(FrameQueue *queue, CanRecord *record)
{
  const unsigned tail = atomic_load_explicit(&queue->tail, memory_order_relaxed);
  if (tail == atomic_load_explicit(&queue->head, memory_order_acquire))
  {
    return false;
  }
  *record = queue->slots[tail];
  atomic_store_explicit(&queue->tail, (tail + 1U) % FRAME_QUEUE_SLOTS, memory_order_release);
  return true;
}

// 返回实际可追加的字节数，不含 NUL。0 表示记录或空间无效。
static inline size_t format_can_record(const CanRecord *record, char *line, size_t capacity)
{
  if (record->length > 8U || record->id > (record->extended ? 0x1FFFFFFFU : 0x7FFU))
  {
    return 0U;
  }
  static const char hex[] = "0123456789ABCDEF";
  char payload[17] = {0};
  if (!record->remote)
  {
    for (unsigned i = 0; i < record->length; ++i)
    {
      payload[2U * i] = hex[record->data[i] >> 4];
      payload[2U * i + 1U] = hex[record->data[i] & 0xFU];
    }
  }
  const int length = snprintf(line, capacity, "%lu,%c,%08lX,%c,%u,%s\r\n",
                              (unsigned long)record->time_ms, record->extended ? 'E' : 'S',
                              (unsigned long)record->id, record->remote ? 'R' : 'D',
                              (unsigned)record->length, payload);
  return length > 0 && (size_t)length < capacity ? (size_t)length : 0U;
}
#endif
