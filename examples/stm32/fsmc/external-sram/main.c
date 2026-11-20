#include "stm32f4xx_hal.h"
#include "sram_check.h"

#define SRAM_BASE_ADDR 0x60000000UL

static SRAM_HandleTypeDef memory;
static volatile HAL_StatusTypeDef init_status;
static volatile SramResult test_result;
static volatile uint32_t checks_finished;

void SysTick_Handler(void) { HAL_IncTick(); }

static uint16_t read_word(void *context, uint32_t word)
{
  const volatile uint16_t *address = (volatile uint16_t *)((uintptr_t)context + 2U * word);
  return *address;
}

static void write_word(void *context, uint32_t word, uint16_t value)
{
  volatile uint16_t *address = (volatile uint16_t *)((uintptr_t)context + 2U * word);
  *address = value;
}

static HAL_StatusTypeDef fsmc_init(void)
{
  // 1. 开时钟。DA[15:0]、A[18:16]、读写、片选与字节选通均交给 AF12。
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_FSMC_CLK_ENABLE();

  GPIO_InitTypeDef pins = {
    .Pin = GPIO_PIN_7, .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL,
    .Speed = GPIO_SPEED_FREQ_HIGH, .Alternate = GPIO_AF12_FSMC,
  };
  HAL_GPIO_Init(GPIOB, &pins);
  pins.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5 | GPIO_PIN_7 |
             GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 |
             GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOD, &pins);
  pins.Pin = GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 |
             GPIO_PIN_10 | GPIO_PIN_11 | GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15;
  HAL_GPIO_Init(GPIOE, &pins);

  // 2. 用允许地址复用的 PSRAM 总线模式配 NE1。外部触发器保持低 16 位地址。
  memory.Instance = FSMC_NORSRAM_DEVICE;
  memory.Extended = FSMC_NORSRAM_EXTENDED_DEVICE;
  memory.Init = (FSMC_NORSRAM_InitTypeDef){
    .NSBank = FSMC_NORSRAM_BANK1,
    .DataAddressMux = FSMC_DATA_ADDRESS_MUX_ENABLE,
    .MemoryType = FSMC_MEMORY_TYPE_PSRAM,
    .MemoryDataWidth = FSMC_NORSRAM_MEM_BUS_WIDTH_16,
    .BurstAccessMode = FSMC_BURST_ACCESS_MODE_DISABLE,
    .WaitSignal = FSMC_WAIT_SIGNAL_DISABLE,
    .WriteOperation = FSMC_WRITE_OPERATION_ENABLE,
    .ExtendedMode = FSMC_EXTENDED_MODE_DISABLE,
    .AsynchronousWait = FSMC_ASYNCHRONOUS_WAIT_DISABLE,
  };
  FSMC_NORSRAM_TimingTypeDef timing = {
    .AddressSetupTime = 6U, .AddressHoldTime = 4U, .DataSetupTime = 18U,
    .BusTurnAroundDuration = 4U, .CLKDivision = 2U, .DataLatency = 2U,
    .AccessMode = FSMC_ACCESS_MODE_A,
  };
  return HAL_SRAM_Init(&memory, &timing, &timing);
}

int main(void)
{
  // 1. 复位后的 16 MHz HSI 提供本例时基。外存就绪后才访问映射区。
  HAL_Init();
  HAL_Delay(2U);
  init_status = fsmc_init();
  if (init_status != HAL_OK) { for (;;) {} }

  // 2. 自检独占这片 SRAM。半字地址按两个字节递增，结果保存在片内 RAM。
  const SramIo io = {
    .context = (void *)(uintptr_t)SRAM_BASE_ADDR, .read = read_word, .write = write_word,
  };
  test_result = sram_check(&io);
  checks_finished = 1U;

  // 3. 停留在现场，调试器用 stage、word、expected 和 actual 定位差异。
  for (;;) { __WFI(); }
}
