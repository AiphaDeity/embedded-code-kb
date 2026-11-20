#ifndef SKY_TOUCH_COORDINATES_H
#define SKY_TOUCH_COORDINATES_H

#include <stdbool.h>
#include <stdint.h>

typedef struct { uint16_t x, y; } TouchPoint;
typedef struct { uint16_t x0, y0, x1, y1; } TouchRect;

static inline uint16_t read_le16(const uint8_t bytes[2])
{
  return (uint16_t)((unsigned)bytes[0] | ((unsigned)bytes[1] << 8));
}

// source/target 的 x、y 各自为宽和高。失败时不修改 result。
static inline bool map_touch(TouchPoint raw, TouchPoint source, TouchPoint target, TouchPoint *result)
{
  if (source.x < 2U || source.y < 2U || target.x == 0U || target.y == 0U ||
      raw.x >= source.x || raw.y >= source.y)
  {
    return false;
  }
  result->x = (uint16_t)((uint32_t)raw.x * (target.x - 1U) / (source.x - 1U));
  result->y = (uint16_t)((uint32_t)raw.y * (target.y - 1U) / (source.y - 1U));
  return true;
}

// point 已在屏幕内，取左右各 4 像素并裁剪到可用范围，端点均包含。
static inline TouchRect touch_marker(TouchPoint point, TouchPoint bounds)
{
  const uint32_t right = (uint32_t)point.x + 4U, bottom = (uint32_t)point.y + 4U;
  return (TouchRect){
    .x0 = point.x >= 4U ? point.x - 4U : 0U,
    .y0 = point.y >= 4U ? point.y - 4U : 0U,
    .x1 = right < bounds.x ? right : bounds.x - 1U,
    .y1 = bottom < bounds.y ? bottom : bounds.y - 1U,
  };
}
#endif
