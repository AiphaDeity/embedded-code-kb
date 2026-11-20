#ifndef SKY_GT911_H
#define SKY_GT911_H

#include "stm32f4xx_hal.h"
#include "coordinates.h"

enum { GT911_ADDRESS = 0x5D << 1, GT_ID = 0x8140, GT_SIZE = 0x8146, GT_STATUS = 0x814E, GT_FIRST_X = 0x8150 };
typedef enum { TOUCH_WAIT, TOUCH_UP, TOUCH_DOWN, TOUCH_BAD, TOUCH_IO } TouchState;
static I2C_HandleTypeDef touch;
static TouchPoint sensor_size;

static HAL_StatusTypeDef touch_read(uint16_t address, uint8_t *bytes, uint16_t length)
{
  return HAL_I2C_Mem_Read(&touch, GT911_ADDRESS, address, I2C_MEMADD_SIZE_16BIT, bytes, length, 30U);
}

static inline bool gt911_init(void)
{
  // 1. 复位期间将 INT 保持低电平选地址 0x5D，芯片启动后把 INT 改回输入。
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_I2C1_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_5, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_RESET);
  GPIO_InitTypeDef pin = {.Pin = GPIO_PIN_5, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOB, &pin);
  pin.Pin = GPIO_PIN_7;
  HAL_GPIO_Init(GPIOC, &pin);
  HAL_Delay(20U);
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_7, GPIO_PIN_SET);
  HAL_Delay(60U);
  pin.Mode = GPIO_MODE_INPUT;
  pin.Pin = GPIO_PIN_5;
  HAL_GPIO_Init(GPIOB, &pin);
  HAL_Delay(100U);

  // 2. I²C1 使用外部上拉。核对产品 ID，再读取控制器报告的坐标尺寸。
  pin = (GPIO_InitTypeDef){
    .Pin = GPIO_PIN_6 | GPIO_PIN_7, .Mode = GPIO_MODE_AF_OD,
    .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF4_I2C1,
  };
  HAL_GPIO_Init(GPIOB, &pin);
  touch.Instance = I2C1;
  touch.Init = (I2C_InitTypeDef){.ClockSpeed = 100000U, .DutyCycle = I2C_DUTYCYCLE_2,
                                 .AddressingMode = I2C_ADDRESSINGMODE_7BIT};
  uint8_t id[4], size[4];
  if (HAL_I2C_Init(&touch) != HAL_OK || touch_read(GT_ID, id, sizeof(id)) != HAL_OK ||
      id[0] != '9' || id[1] != '1' || id[2] != '1' ||
      touch_read(GT_SIZE, size, sizeof(size)) != HAL_OK)
  {
    return false;
  }
  sensor_size = (TouchPoint){read_le16(size), read_le16(size + 2U)};
  return sensor_size.x >= 2U && sensor_size.y >= 2U;
}

static inline TouchState gt911_next(TouchPoint *point, TouchPoint target)
{
  // 1. 没有新数据就保留触点。新数据才确定是抬起、按下还是坏包。
  uint8_t status;
  if (touch_read(GT_STATUS, &status, 1U) != HAL_OK) { return TOUCH_IO; }
  if (!(status & 0x80U)) { return TOUCH_WAIT; }
  const unsigned count = status & 0x0FU;
  uint8_t xy[4];
  if (count != 0U && count <= 5U && touch_read(GT_FIRST_X, xy, sizeof(xy)) != HAL_OK)
  {
    return TOUCH_IO;
  }

  // 2. 坐标读取成功后清状态。只有芯片接收了清零，才交付这帧给渲染。
  uint8_t zero = 0U;
  if (HAL_I2C_Mem_Write(&touch, GT911_ADDRESS, GT_STATUS, I2C_MEMADD_SIZE_16BIT, &zero, 1U, 30U) != HAL_OK)
  {
    return TOUCH_IO;
  }
  if (count == 0U) { return TOUCH_UP; }
  if (count > 5U) { return TOUCH_BAD; }
  const TouchPoint raw = {read_le16(xy), read_le16(xy + 2U)};
  return map_touch(raw, sensor_size, target, point) ? TOUCH_DOWN : TOUCH_BAD;
}

#endif
