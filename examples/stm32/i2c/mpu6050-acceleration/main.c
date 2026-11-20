#include "stm32f4xx_hal.h"
#include "acceleration.h"
#include <stdbool.h>

enum { ADDRESS = 0x68U << 1, WHO_AM_I = 0x75, PWR_MGMT_1 = 0x6B,
       ACCEL_CONFIG = 0x1C, ACCEL_XOUT_H = 0x3B, TIMEOUT_MS = 50U };
enum { BUS_INIT, IDENTITY, WAKE, RANGE, BURST_READ };
typedef enum { IO_OK, IO_NACK, IO_TIMEOUT, IO_BUSY, IO_OTHER, BAD_ID } Outcome;

static I2C_HandleTypeDef bus;
static volatile unsigned phase;
static volatile HAL_StatusTypeDef last_hal_status;
static volatile uint32_t last_hal_error;
static volatile Outcome outcome;
static volatile Acceleration latest;
static volatile bool sample_valid;
static volatile uint32_t good_samples, failed_rounds;

void SysTick_Handler(void) { HAL_IncTick(); }

static HAL_StatusTypeDef bus_init(void)
{
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_I2C1_CLK_ENABLE();
  GPIO_InitTypeDef pins = {
    .Pin = GPIO_PIN_6 | GPIO_PIN_7, .Mode = GPIO_MODE_AF_OD,
    .Speed = GPIO_SPEED_FREQ_LOW, .Alternate = GPIO_AF4_I2C1,
  };
  HAL_GPIO_Init(GPIOB, &pins);
  bus.Instance = I2C1;
  bus.Init = (I2C_InitTypeDef){
    .ClockSpeed = 100000U, .DutyCycle = I2C_DUTYCYCLE_2,
    .AddressingMode = I2C_ADDRESSINGMODE_7BIT,
  };
  return HAL_I2C_Init(&bus);
}

static bool capture_status(HAL_StatusTypeDef status)
{
  // 1. 保留 HAL 的返回值和位掩码。BUSY 可能在事务开始前返回，优先按 BUSY 处理。
  const uint32_t error = HAL_I2C_GetError(&bus);
  last_hal_status = status;
  last_hal_error = error;
  if (status == HAL_OK) { outcome = IO_OK; }
  else if (status == HAL_BUSY) { outcome = IO_BUSY; }
  else if (status == HAL_TIMEOUT || (error & HAL_I2C_ERROR_TIMEOUT)) { outcome = IO_TIMEOUT; }
  else if (error & HAL_I2C_ERROR_AF) { outcome = IO_NACK; }
  else { outcome = IO_OTHER; }
  return status == HAL_OK;
}

static bool prepare_sensor(void)
{
  // 1. 先核对身份。未识别到器件时，不写入任何配置寄存器。
  uint8_t id;
  phase = IDENTITY;
  if (!capture_status(HAL_I2C_Mem_Read(&bus, ADDRESS, WHO_AM_I, I2C_MEMADD_SIZE_8BIT,
                                     &id, 1U, TIMEOUT_MS))) { return false; }
  if (id != 0x68U) { outcome = BAD_ID; return false; }

  // 2. 退出休眠并选 X 陀螺 PLL。再设加速度量程为 ±2 g，供主循环解释原始值。
  uint8_t setting = 0x01U;
  phase = WAKE;
  if (!capture_status(HAL_I2C_Mem_Write(&bus, ADDRESS, PWR_MGMT_1, I2C_MEMADD_SIZE_8BIT,
                                      &setting, 1U, TIMEOUT_MS))) { return false; }
  setting = 0U;
  phase = RANGE;
  if (!capture_status(HAL_I2C_Mem_Write(&bus, ADDRESS, ACCEL_CONFIG, I2C_MEMADD_SIZE_8BIT,
                                      &setting, 1U, TIMEOUT_MS))) { return false; }
  HAL_Delay(100U);
  return true;
}

static bool read_acceleration(void)
{
  // 1. 合并读从 0x3B 起的六个字节。若 HAL 报错，不发布可能读了一半的数据。
  uint8_t bytes[6];
  phase = BURST_READ;
  if (!capture_status(HAL_I2C_Mem_Read(&bus, ADDRESS, ACCEL_XOUT_H, I2C_MEMADD_SIZE_8BIT,
                                     bytes, sizeof(bytes), TIMEOUT_MS))) { return false; }

  // 2. 把三个轴一起交给调试器。每个轴单位为原始计数，当前量程为 16384 LSB/g。
  latest = accel_decode(bytes);
  return true;
}

int main(void)
{
  // 1. 初始化 HAL 与 100 kHz I²C。控制器初始化失败时停住。
  HAL_Init();
  if (!capture_status(bus_init())) { for (;;) {} }
  HAL_Delay(100U);

  bool configured = false;
  for (;;)
  {
    // 2. 首次或上一轮失败时先确认器件并配置。配置好才允许读取。
    if (!configured) { configured = prepare_sensor(); }
    sample_valid = configured && read_acceleration();
    if (sample_valid)
    {
      ++good_samples;
    }
    else
    {
      // 3. 保留故障状态和上次样本，标为不可用。下一轮重新确认地址和配置。
      ++failed_rounds;
      configured = false;
    }
    HAL_Delay(250U);
  }
}
