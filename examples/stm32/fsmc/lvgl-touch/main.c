#include "lvgl.h"
#include "../common/ili9341.h"
#include "../ili9341-touch/gt911.h"

#if LVGL_VERSION_MAJOR != 9 || LVGL_VERSION_MINOR != 6
#error Use LVGL 9.6 for this example
#endif

enum { DRAW_LINES = 12 };
static _Alignas(4) uint16_t pixels[LCD_WIDTH * DRAW_LINES];
static const TouchPoint screen = {LCD_WIDTH, LCD_HEIGHT};
static volatile uint32_t stage, flushes, bad_windows, input_errors, presses;
static lv_obj_t *label;
void SysTick_Handler(void) { HAL_IncTick(); }

static void flush_region(lv_display_t *display, const lv_area_t *area, uint8_t *data)
{
  // 1. partial 模式交付一个已绘制矩形。坐标有效且像素在缓冲内才写 FSMC。
  const int32_t width = lv_area_get_width(area), height = lv_area_get_height(area);
  if (area->x1 >= 0 && area->y1 >= 0 && area->x2 < LCD_WIDTH && area->y2 < LCD_HEIGHT &&
      width > 0 && height > 0 && width * height <= LCD_WIDTH * DRAW_LINES &&
      ili9341_window((uint16_t)area->x1, (uint16_t)area->y1, (uint16_t)area->x2, (uint16_t)area->y2))
  {
    const uint16_t *rgb565 = (const uint16_t *)data;
    for (int32_t i = 0; i < width * height; ++i) { LCD_DATA = rgb565[i]; }
    ++flushes;
  }
  else
  {
    ++bad_windows;
  }

  // 2. 同步刷屏完成后才归还缓冲。无效窗口也应结束本次交付。
  lv_display_flush_ready(display);
}

static void read_pointer(lv_indev_t *device, lv_indev_data_t *data)
{
  (void)device;
  static TouchPoint point;
  static bool pressed;
  TouchPoint latest;

  // 1. 只用新数据推进坐标或状态。没有新帧时重复交出上一状态。
  switch (gt911_next(&latest, screen))
  {
    case TOUCH_DOWN: point = latest; pressed = true; break;
    case TOUCH_UP: pressed = false; break;
    case TOUCH_IO:
    case TOUCH_BAD: ++input_errors; break;
    case TOUCH_WAIT: break;
  }
  data->point.x = point.x;
  data->point.y = point.y;
  data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
}

static void on_click(lv_event_t *event)
{
  (void)event;
  // 1. LVGL 在按下并松开形成点击后调用这里。更新标签会使对应区域失效，随后重新绘制。
  ++presses;
  lv_label_set_text_fmt(label, "Count: %lu", (unsigned long)presses);
}

int main(void)
{
  // 1. 先准备实际屏幕和触摸模块，再把两个接口交给 LVGL。
  HAL_Init();
  stage = 1U;
  if (ili9341_init() != HAL_OK) { for (;;) {} }
  stage = 2U;
  if (!gt911_init()) { for (;;) {} }
  lv_init();
  lv_tick_set_cb(HAL_GetTick);

  // 2. 一份 RGB565 部分缓冲负责输出。触摸设备挂到同一屏幕。
  stage = 3U;
  lv_display_t *display = lv_display_create(LCD_WIDTH, LCD_HEIGHT);
  lv_indev_t *pointer = lv_indev_create();
  if (display == NULL || pointer == NULL) { for (;;) {} }
  lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
  lv_display_set_buffers(display, pixels, NULL, sizeof(pixels), LV_DISPLAY_RENDER_MODE_PARTIAL);
  lv_display_set_flush_cb(display, flush_region);
  lv_indev_set_type(pointer, LV_INDEV_TYPE_POINTER);
  lv_indev_set_display(pointer, display);
  lv_indev_set_read_cb(pointer, read_pointer);

  // 3. 实际按钮点击会更新标签。main 持续推进 LVGL，所有界面操作都在同一上下文。
  lv_obj_t *button = lv_button_create(lv_display_get_screen_active(display));
  lv_obj_set_size(button, 136, 56);
  lv_obj_center(button);
  lv_obj_add_event_cb(button, on_click, LV_EVENT_CLICKED, NULL);
  label = lv_label_create(button);
  lv_label_set_text(label, "Count: 0");
  lv_obj_center(label);
  stage = 0U;
  for (;;)
  {
    lv_timer_handler();
    HAL_Delay(5U);
  }
}
