#include "rx_ring.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
  UartRing ring = {0};
  uint8_t byte = 0xAA;
  assert(!uart_ring_pop(&ring, &byte) && byte == 0xAA);
  for (unsigned i = 0; i < UART_RING_CAPACITY; ++i)
  {
    assert(uart_ring_push(&ring, (uint8_t)i));
  }
  assert(!uart_ring_push(&ring, 0xFF));
  for (unsigned i = 0; i < 15U; ++i)
  {
    assert(uart_ring_pop(&ring, &byte) && byte == i);
  }
  for (unsigned i = UART_RING_CAPACITY; i < UART_RING_CAPACITY + 15U; ++i)
  {
    assert(uart_ring_push(&ring, (uint8_t)i));
  }
  for (unsigned i = 15U; i < UART_RING_CAPACITY + 15U; ++i)
  {
    assert(uart_ring_pop(&ring, &byte) && byte == i);
  }
  assert(!uart_ring_pop(&ring, &byte) && byte == UART_RING_CAPACITY + 14U);
  puts("UART RX ring: empty, overflow, wrap and ordered consumption");
}
