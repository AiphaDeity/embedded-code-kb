#include "../../stm32/fsmc/common/ili9341.h"
#include "sample.h"

enum { ADDRESS = 0x44U << 1, BAR_X = 20U, BAR_WIDTH = 200U };
enum { SAMPLE_OK, SAMPLE_TX_FAILED, SAMPLE_RX_FAILED, SAMPLE_BAD_CRC };

static I2C_HandleTypeDef bus;
static volatile HAL_StatusTypeDef lcd_init_status;
static volatile HAL_StatusTypeDef i2c_init_status;
static volatile unsigned sample_status;
static volatile uint32_t samples_ok;
static volatile uint32_t samples_failed;
static volatile Sht30Sample last_sample;

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

static unsigned measure(Sht30Sample *sample)
{
  // 1. 启动一次高重复性、无时钟拉伸的测量。发送失败时这轮到此结束。
  uint8_t command[] = {0x24U, 0x00U};
  if (HAL_I2C_Master_Transmit(&bus, ADDRESS, command, sizeof(command), 30U) != HAL_OK)
  {
    return SAMPLE_TX_FAILED;
  }

  // 2. 转换完成后一次读六个字节。两组 CRC 均通过才把本轮温湿度交给主循环。
  HAL_Delay(20U);
  uint8_t frame[6];
  if (HAL_I2C_Master_Receive(&bus, ADDRESS, frame, sizeof(frame), 30U) != HAL_OK)
  {
    return SAMPLE_RX_FAILED;
  }
  return sht30_decode(frame, sample) ? SAMPLE_OK : SAMPLE_BAD_CRC;
}

static void draw_bar(uint16_t y, uint16_t filled, uint16_t color)
{
  if (filled > 0)
  {
    ili9341_fill(BAR_X, y, BAR_X + filled - 1U, y + 19U, color);
  }
  if (filled < BAR_WIDTH)
  {
    ili9341_fill(BAR_X + filled, y, BAR_X + BAR_WIDTH - 1U, y + 19U, 0x2104U);
  }
}

int main(void)
{
  // 1. 初始化时基和两条总线。任一初始化失败时保留状态，由调试器读取。
  HAL_Init();
  lcd_init_status = ili9341_init();
  i2c_init_status = bus_init();
  if (lcd_init_status != HAL_OK || i2c_init_status != HAL_OK)
  {
    for (;;) {}
  }
  ili9341_fill(0U, 0U, LCD_WIDTH - 1U, LCD_HEIGHT - 1U, 0x0000U);

  for (;;)
  {
    // 2. measure 交付同一轮的两项有效读数。主循环据此更新两行条形图和调试快照。
    Sht30Sample next;
    sample_status = measure(&next);
    if (sample_status == SAMPLE_OK)
    {
      last_sample = next;
      ++samples_ok;
      draw_bar(50U, bar_width(next.temperature_tenths, 500U, BAR_WIDTH), 0xFD20U);
      draw_bar(100U, bar_width(next.humidity_tenths, 1000U, BAR_WIDTH), 0x07FFU);
      ili9341_fill(20U, 145U, 39U, 164U, 0x07E0U);
    }
    else
    {
      // 3. 失败时保留上次的条形图，红色状态块提示它们已过期。下一轮继续采样。
      ++samples_failed;
      ili9341_fill(20U, 145U, 39U, 164U, 0xF800U);
    }
    HAL_Delay(500U);
  }
}
