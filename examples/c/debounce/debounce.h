#ifndef KB_DEBOUNCE_H
#define KB_DEBOUNCE_H

#include <stdbool.h>
#include <stdint.h>

typedef enum { KEY_IDLE, KEY_PRESSED, KEY_RELEASED } KeyEvent;
typedef struct
{
  bool stable;
  bool candidate;
  uint32_t changed_at;
} Debounce;

// 用已知的初始稳定电平初始化。每轮传入按键原始值和毫秒计数，间隔须小于时基完整回绕周期。
static inline KeyEvent debounce_update(Debounce *key, bool raw, uint32_t now, uint32_t stable_ms)
{
  // 1. 每次观察到原始电平变动，就重新记候选电平及起点。
  if (raw != key->candidate)
  {
    key->candidate = raw;
    key->changed_at = now;
  }

  // 2. 只有候选电平持续够久且不同于旧稳定值，才交给调用者一个边沿事件。
  if (key->candidate != key->stable && (uint32_t)(now - key->changed_at) >= stable_ms)
  {
    key->stable = key->candidate;
    return key->stable ? KEY_PRESSED : KEY_RELEASED;
  }
  return KEY_IDLE;
}

#endif
