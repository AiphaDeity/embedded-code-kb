#include <assert.h>
#include <stdio.h>
#include "coordinates.h"

int main(void)
{
  // 1. 触摸原点和最大有效坐标分别映射到屏幕两角。
  const TouchPoint source = {800U, 480U}, screen = {240U, 320U};
  TouchPoint point;
  assert(map_touch((TouchPoint){0U, 0U}, source, screen, &point) && point.x == 0U && point.y == 0U);
  assert(map_touch((TouchPoint){799U, 479U}, source, screen, &point) && point.x == 239U && point.y == 319U);
  assert(map_touch((TouchPoint){0U, 479U}, source, screen, &point) && point.x == 0U && point.y == 319U);
  assert(map_touch((TouchPoint){799U, 0U}, source, screen, &point) && point.x == 239U && point.y == 0U);
  assert(!map_touch((TouchPoint){800U, 479U}, source, screen, &point) && point.x == 239U);
  // 2. 屏幕边缘只刷新可见部分，中间位置仍保持完整 9 × 9。
  TouchRect rect = touch_marker((TouchPoint){239U, 319U}, screen);
  assert(rect.x0 == 235U && rect.x1 == 239U && rect.y0 == 315U && rect.y1 == 319U);
  rect = touch_marker((TouchPoint){0U, 0U}, screen);
  assert(rect.x0 == 0U && rect.x1 == 4U && rect.y0 == 0U && rect.y1 == 4U);
  rect = touch_marker((TouchPoint){120U, 160U}, screen);
  assert(rect.x1 - rect.x0 == 8U && rect.y1 - rect.y0 == 8U);
  puts("GT911 mapping: corners, invalid sample and clipped marker");
  return 0;
}
