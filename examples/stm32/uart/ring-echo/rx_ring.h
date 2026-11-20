#ifndef KB_UART_RX_RING_H
#define KB_UART_RX_RING_H

#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

enum { UART_RING_SLOTS = 32, UART_RING_CAPACITY = UART_RING_SLOTS - 1 };

typedef struct
{
  uint8_t data[UART_RING_SLOTS];
  atomic_uint head, tail;
} UartRing;

_Static_assert(ATOMIC_INT_LOCK_FREE == 2, "RX ISR requires lock-free indices");

// 仅 USART1 IRQ 生产。成功前先写数据，再发布 head。满时保留已有内容。
static inline bool uart_ring_push(UartRing *ring, uint8_t byte)
{
  const unsigned head = atomic_load_explicit(&ring->head, memory_order_relaxed);
  const unsigned next = (head + 1U) % UART_RING_SLOTS;
  if (next == atomic_load_explicit(&ring->tail, memory_order_acquire))
  {
    return false;
  }
  ring->data[head] = byte;
  atomic_store_explicit(&ring->head, next, memory_order_release);
  return true;
}

// 仅主循环消费。先复制字节，再发布 tail。空时保持输出参数不变。
static inline bool uart_ring_pop(UartRing *ring, uint8_t *byte)
{
  const unsigned tail = atomic_load_explicit(&ring->tail, memory_order_relaxed);
  if (tail == atomic_load_explicit(&ring->head, memory_order_acquire))
  {
    return false;
  }
  *byte = ring->data[tail];
  atomic_store_explicit(&ring->tail, (tail + 1U) % UART_RING_SLOTS, memory_order_release);
  return true;
}

#endif
