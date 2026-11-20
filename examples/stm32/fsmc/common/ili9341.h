#ifndef SKY_ILI9341_H
#define SKY_ILI9341_H

#include <stdbool.h>
#include "stm32f4xx_hal.h"

#define LCD_COMMAND (*(volatile uint16_t *)0x60000000UL)
#define LCD_DATA (*(volatile uint16_t *)0x60020000UL)
enum { LCD_WIDTH = 240, LCD_HEIGHT = 320 };

static inline HAL_StatusTypeDef ili9341_bus_init(void)
{
  static SRAM_HandleTypeDef lcd_bus;

  // 1. 使能并口引脚与 FSMC。PC6 在总线准备期间拉低屏幕的复位脚。
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_FSMC_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_RESET);
  GPIO_InitTypeDef reset = {.Pin = GPIO_PIN_6, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOC, &reset);

  GPIO_InitTypeDef bus = {
    .Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7 |
           GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_14 | GPIO_PIN_15,
    .Mode = GPIO_MODE_AF_PP, .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF12_FSMC,
  };
  HAL_GPIO_Init(GPIOD, &bus);
  bus.Pin = GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
            GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOE, &bus);

  // 2. 16 位非复用异步总线。用低速保守时序，命令和数据共用写周期。
  lcd_bus.Instance = FSMC_NORSRAM_DEVICE;
  lcd_bus.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
  lcd_bus.Init = (FSMC_NORSRAM_InitTypeDef){
    .NSBank = FSMC_NORSRAM_BANK1, .DataAddressMux = FSMC_DATA_ADDRESS_MUX_DISABLE,
    .MemoryType = FSMC_MEMORY_TYPE_SRAM, .MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16,
    .WriteOperation = FSMC_WRITE_OPERATION_ENABLE,
  };
  FSMC_NORSRAM_TimingTypeDef timing = {
    .AddressSetupTime = 4U, .AddressHoldTime = 1U, .DataSetupTime = 12U,
    .CLKDivision = 2U, .DataLatency = 2U, .AccessMode = FSMC_ACCESS_MODE_A,
  };
  return HAL_SRAM_Init(&lcd_bus, &timing, &timing);
}

static inline HAL_StatusTypeDef ili9341_init(void)
{
  // 1. 平行总线准备好才解除复位。面板退出休眠后使用 RGB565。
  const HAL_StatusTypeDef status = ili9341_bus_init();
  if (status != HAL_OK) { return status; }
  HAL_Delay(20U);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6, GPIO_PIN_SET);
  HAL_Delay(120U);
  LCD_COMMAND = 0x11U;
  HAL_Delay(120U);
  LCD_COMMAND = 0x36U;
  LCD_DATA = 0x00U;
  LCD_COMMAND = 0x3AU;
  LCD_DATA = 0x55U;
  LCD_COMMAND = 0x29U;
  return HAL_OK;
}

static inline void lcd_coordinate(uint16_t value)
{
  LCD_DATA = value >> 8;
  LCD_DATA = value & 0xFFU;
}

// 坐标是包含两端的矩形。无效参数不访问总线。
static inline bool ili9341_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
  if (x0 > x1 || y0 > y1 || x1 >= LCD_WIDTH || y1 >= LCD_HEIGHT)
  {
    return false;
  }
  // 1. 列、行分别写入起止地址，再发写显存命令。后续总线数据按行推进。
  LCD_COMMAND = 0x2AU;
  lcd_coordinate(x0);
  lcd_coordinate(x1);
  LCD_COMMAND = 0x2BU;
  lcd_coordinate(y0);
  lcd_coordinate(y1);
  LCD_COMMAND = 0x2CU;
  return true;
}

static inline void ili9341_fill(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1, uint16_t color)
{
  // 1. 每个逻辑像素一次半字写。像素总数严格匹配窗口面积。
  if (!ili9341_window(x0, y0, x1, y1)) { return; }
  const uint32_t pixels = ((uint32_t)x1 + 1U - x0) * ((uint32_t)y1 + 1U - y0);
  for (uint32_t i = 0; i < pixels; ++i) { LCD_DATA = color; }
}

#endif
