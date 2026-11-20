#include "stm32f4xx_hal.h"
#include "../../../c/debounce/debounce.h"
#include "store.h"

static SPI_HandleTypeDef spi;
static volatile uint32_t active_period, active_slot, generation, failed_saves, init_error;
void SysTick_Handler(void) { HAL_IncTick(); }

static bool transfer(const uint8_t *command, size_t command_size, const uint8_t *output,
                     uint8_t *input, size_t data_size)
{
  // 1. 每个 SPI 命令有独立片选。命令、地址及可能的载荷在同一个事务内。
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_RESET);
  HAL_StatusTypeDef status = HAL_SPI_Transmit(&spi, command, (uint16_t)command_size, 20U);
  if (status == HAL_OK && output != NULL)
  {
    status = HAL_SPI_Transmit(&spi, output, (uint16_t)data_size, 20U);
  }
  if (status == HAL_OK && input != NULL)
  {
    uint8_t dummy[NOR_RECORD] = {0};
    status = HAL_SPI_TransmitReceive(&spi, dummy, input, (uint16_t)data_size, 20U);
  }
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  return status == HAL_OK;
}

static bool read_status(uint8_t *value)
{
  const uint8_t command = 0x05U;
  return transfer(&command, 1U, NULL, value, 1U);
}

static bool wait_ready(uint32_t limit_ms)
{
  const uint32_t start = HAL_GetTick();
  do
  {
    uint8_t flags;
    if (!read_status(&flags)) { return false; }
    if ((flags & 1U) == 0U) { return true; }
    HAL_Delay(1U);
  } while ((uint32_t)(HAL_GetTick() - start) < limit_ms);
  return false;
}

static bool write_enable(void)
{
  uint8_t status;
  const uint8_t command = 0x06U;
  return wait_ready(1000U) && transfer(&command, 1U, NULL, NULL, 0U) &&
         read_status(&status) && (status & 2U) != 0U;
}

// 参数区固定在最后两个扇区。所有写入仍由各命令的页/扇区边界约束。
static bool permitted(uint32_t address, size_t size)
{
  return address >= NOR_SLOT_A && address < 0x1000000U && size <= 0x1000000U - address;
}

static bool nor_read_hw(void *context, uint32_t address, uint8_t *data, size_t size)
{
  (void)context;
  if (!permitted(address, size) || size > NOR_RECORD || !wait_ready(1000U)) { return false; }
  const uint8_t command[] = {0x03U, (uint8_t)(address >> 16), (uint8_t)(address >> 8), (uint8_t)address};
  return transfer(command, sizeof(command), NULL, data, size);
}

static bool nor_program_hw(void *context, uint32_t address, const uint8_t *data, size_t size)
{
  (void)context;
  const uint8_t command[] = {0x02U, (uint8_t)(address >> 16), (uint8_t)(address >> 8), (uint8_t)address};
  return size > 0U && permitted(address, size) && size <= 256U - (address & 255U) &&
         write_enable() && transfer(command, sizeof(command), data, NULL, size) && wait_ready(20U);
}

static bool nor_erase_hw(void *context, uint32_t address)
{
  (void)context;
  const uint8_t command[] = {0x20U, (uint8_t)(address >> 16), (uint8_t)(address >> 8), (uint8_t)address};
  return (address == NOR_SLOT_A || address == NOR_SLOT_B) &&
         write_enable() && transfer(command, sizeof(command), NULL, NULL, 0U) && wait_ready(1000U);
}

static bool init_board(void)
{
  // 1. SPI1 Mode 0，PA4 默认释放片选。PA0/PB2 为输入和观察信号。
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_SPI1_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_4, GPIO_PIN_SET);
  GPIO_InitTypeDef pins = {.Pin = GPIO_PIN_4, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOA, &pins);
  pins = (GPIO_InitTypeDef){.Pin = GPIO_PIN_0, .Mode = GPIO_MODE_INPUT};
  HAL_GPIO_Init(GPIOA, &pins);
  pins = (GPIO_InitTypeDef){.Pin = GPIO_PIN_2, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOB, &pins);
  pins = (GPIO_InitTypeDef){
    .Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7, .Mode = GPIO_MODE_AF_PP,
    .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF5_SPI1,
  };
  HAL_GPIO_Init(GPIOA, &pins);
  spi.Instance = SPI1;
  spi.Init = (SPI_InitTypeDef){
    .Mode = SPI_MODE_MASTER, .Direction = SPI_DIRECTION_2LINES, .DataSize = SPI_DATASIZE_8BIT,
    .CLKPolarity = SPI_POLARITY_LOW, .CLKPhase = SPI_PHASE_1EDGE, .NSS = SPI_NSS_SOFT,
    .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16, .FirstBit = SPI_FIRSTBIT_MSB,
  };

  // 2. 等前一次可能尚未结束的写入，再核对 W25Q128 的 JEDEC ID。
  const uint8_t command = 0x9FU;
  uint8_t id[3];
  return HAL_SPI_Init(&spi) == HAL_OK && wait_ready(1000U) &&
         transfer(&command, 1U, NULL, id, sizeof(id)) &&
         id[0] == 0xEFU && id[1] == 0x40U && id[2] == 0x18U;
}

int main(void)
{
  // 1. 读取 A/B 槽。初值和最新提交由同一套规则恢复，SPI 失败则停机。
  HAL_Init();
  const NorIo io = {.read = nor_read_hw, .program = nor_program_hw, .erase_sector = nor_erase_hw};
  NorSetting setting;
  init_error = 1U;
  if (!init_board()) { for (;;) {} }
  init_error = 2U;
  if (!nor_load(&io, &setting)) { for (;;) {} }
  init_error = 0U;

  Debounce key = {0};
  uint32_t last = HAL_GetTick();
  for (;;)
  {
    // 2. 按键稳定后提交新的值。失败继续用之前的参数，下一次另行尝试。
    const uint32_t now = HAL_GetTick();
    const bool pressed = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET;
    if (debounce_update(&key, pressed, now, 20U) == KEY_PRESSED)
    {
      const uint32_t period = setting.period_ms > 900U ? 100U : setting.period_ms + 100U;
      if (!nor_save(&io, period, &setting))
      {
        ++failed_saves;
      }
    }

    // 3. 让读回的参数控制板载 LED。调试变量同时显示生效的槽与代数。
    active_period = setting.period_ms;
    active_slot = setting.address;
    generation = setting.generation;
    if ((uint32_t)(now - last) >= setting.period_ms)
    {
      last = now;
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
    }
    __WFI();
  }
}
