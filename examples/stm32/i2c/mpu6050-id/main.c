#include "stm32f4xx_hal.h"

#define MPU_ADDRESS_7BIT 0x68U
#define WHO_AM_I 0x75U

static I2C_HandleTypeDef bus;
static volatile HAL_StatusTypeDef status;
static volatile uint32_t i2c_error;
static volatile uint8_t id;

void SysTick_Handler(void) { HAL_IncTick(); }

int main(void)
{
  // 1. 先打开 GPIO 和 I2C 时钟，再由 AF4 接管开漏引脚。
  HAL_Init();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_I2C1_CLK_ENABLE();
  GPIO_InitTypeDef pins = {
    .Pin = GPIO_PIN_6 | GPIO_PIN_7, .Mode = GPIO_MODE_AF_OD,
    .Speed = GPIO_SPEED_FREQ_LOW, .Alternate = GPIO_AF4_I2C1,
  };
  HAL_GPIO_Init(GPIOB, &pins);

  // 2. 选择标准模式 100 kHz 和七位地址。
  bus.Instance = I2C1;
  bus.Init = (I2C_InitTypeDef){
    .ClockSpeed = 100000U, .DutyCycle = I2C_DUTYCYCLE_2,
    .AddressingMode = I2C_ADDRESSINGMODE_7BIT,
  };
  status = HAL_I2C_Init(&bus);
  if (status != HAL_OK) { for (;;) {} }
  HAL_Delay(100U);

  for (;;)
  {
    // 3. HAL 先发送寄存器地址，再改用读方向。用本轮状态区分旧读数和新数据。
    uint8_t value;
    status = HAL_I2C_Mem_Read(&bus, MPU_ADDRESS_7BIT << 1U, WHO_AM_I,
                             I2C_MEMADD_SIZE_8BIT, &value, 1U, 50U);
    i2c_error = HAL_I2C_GetError(&bus);
    if (status == HAL_OK) { id = value; }
    HAL_Delay(250U);
  }
}
