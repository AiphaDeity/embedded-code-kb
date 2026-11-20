#include <assert.h>
#include <stdio.h>
#include "debounce.h"

int main(void)
{
  Debounce key = {0};
  // 1. 模拟按下时抖动，并跨过时基回绕。最后一次上升才开始 20 ms 的确认。
  assert(debounce_update(&key, true, UINT32_MAX - 16U, 20U) == KEY_IDLE);
  assert(debounce_update(&key, false, UINT32_MAX - 12U, 20U) == KEY_IDLE);
  assert(debounce_update(&key, true, UINT32_MAX - 9U, 20U) == KEY_IDLE);
  assert(debounce_update(&key, true, 9U, 20U) == KEY_IDLE);
  assert(debounce_update(&key, true, 10U, 20U) == KEY_PRESSED);

  // 2. 持续按住只产生一次事件。松开稳定 20 ms 后才报告释放。
  assert(debounce_update(&key, true, 50U, 20U) == KEY_IDLE);
  assert(debounce_update(&key, false, 51U, 20U) == KEY_IDLE);
  assert(debounce_update(&key, false, 71U, 20U) == KEY_RELEASED);
  puts("debounce: bounce, tick wrap, one press, one release");
  return 0;
}
