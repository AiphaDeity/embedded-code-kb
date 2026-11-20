#include "../common/ili9341.h"
#include "gt911.h"

static const TouchPoint display = {LCD_WIDTH, LCD_HEIGHT};
static volatile uint32_t stage, io_errors, bad_samples, drawings;
static volatile TouchPoint last_point;
void SysTick_Handler(void) { HAL_IncTick(); }

int main(void)
{
  // 1. LCD 先清为黑色背景。触摸初始化失败时保留阶段号。
  HAL_Init();
  stage = 1U;
  if (ili9341_init() != HAL_OK) { for (;;) {} }
  ili9341_fill(0U, 0U, LCD_WIDTH - 1U, LCD_HEIGHT - 1U, 0x0000U);
  stage = 2U;
  if (!gt911_init()) { for (;;) {} }
  stage = 0U;

  bool shown = false;
  TouchRect previous = {0};
  for (;;)
  {
    // 2. 本轮只有新的有效坐标或抬起才更新显示，通信失败保留上一笔。
    TouchPoint point;
    const TouchState state = gt911_next(&point, display);
    if (state == TOUCH_IO) { ++io_errors; }
    if (state == TOUCH_BAD) { ++bad_samples; }
    if ((state == TOUCH_UP || state == TOUCH_DOWN) && shown)
    {
      ili9341_fill(previous.x0, previous.y0, previous.x1, previous.y1, 0x0000U);
      shown = false;
    }

    // 3. 按下时只刷新新标记覆盖的小矩形。框裁剪到屏幕边界。
    if (state == TOUCH_DOWN)
    {
      previous = touch_marker(point, display);
      ili9341_fill(previous.x0, previous.y0, previous.x1, previous.y1, 0xFFFFU);
      shown = true;
      last_point = point;
      ++drawings;
    }
    HAL_Delay(10U);
  }
}
