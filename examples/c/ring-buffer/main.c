#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

static void check(bool ok)
{
  if (!ok) { fputs("self-check failed\n", stderr); exit(EXIT_FAILURE); }
}

#define CAPACITY 4U

typedef struct
{
  uint8_t data[CAPACITY];
  size_t read, write, count;
} Ring;

static bool ring_push(Ring *ring, uint8_t byte)
{
  // 1. 当前没有空位时，拒绝新字节并保留已有顺序。
  if (ring->count == CAPACITY) { return false; }
  // 2. 写到尾部，再推进到下一个可写位置。
  ring->data[ring->write] = byte;
  ring->write = (ring->write + 1U) % CAPACITY;
  ++ring->count;
  return true;
}

static bool ring_pop(Ring *ring, uint8_t *out)
{
  // 1. 空缓冲区保持输出参数原值。
  if (ring->count == 0U) { return false; }
  // 2. 交出最早的字节，然后释放对应位置。
  *out = ring->data[ring->read];
  ring->read = (ring->read + 1U) % CAPACITY;
  --ring->count;
  return true;
}

int main(void)
{
  // 1. 写满四项，第五项必须被拒绝。
  Ring ring = {0};
  for (uint8_t i = 1U; i <= CAPACITY; ++i) { check(ring_push(&ring, i)); }
  check(!ring_push(&ring, 99U));

  // 2. 取两项，再写两项，跨过数组末尾。
  uint8_t received = 0U;
  check(ring_pop(&ring, &received) && received == 1U);
  check(ring_pop(&ring, &received) && received == 2U);
  check(ring_push(&ring, 5U));
  check(ring_push(&ring, 6U));

  // 3. 确认剩余数据严格按先入先出排列，读空后保留原值。
  for (uint8_t i = 3U; i <= 6U; ++i) { check(ring_pop(&ring, &received) && received == i); }
  check(!ring_pop(&ring, &received) && received == 6U);
  puts("ring buffer: OK");
  return 0;
}
