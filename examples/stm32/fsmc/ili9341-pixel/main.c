#include "../common/ili9341.h"

#define SIDE 32U
static volatile HAL_StatusTypeDef bus_status;

void SysTick_Handler(void) { HAL_IncTick(); }

int main(void)
{
  // 1. 初始化 FSMC 与屏幕。失败时保留 HAL 状态供调试。
  HAL_Init();
  bus_status = ili9341_init();
  if (bus_status != HAL_OK) { for (;;) {} }

  uint16_t color = 0xF800U;
  for (;;)
  {
    // 2. 窗口包含两端坐标。写完一块后等待 1 秒，切换下一轮颜色。
    ili9341_fill(0U, 0U, SIDE - 1U, SIDE - 1U, color);
    color = color == 0xF800U ? 0x07E0U : 0xF800U;
    HAL_Delay(1000U);
  }
}
