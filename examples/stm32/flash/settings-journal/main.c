#include "stm32f4xx_hal.h"
#include "../../../c/debounce/debounce.h"
#include "journal.h"

#define SECTOR_ADDRESS 0x08060000U
#define SECTOR_BYTES 0x20000U
#define SLOT_COUNT (SECTOR_BYTES / (WORDS_PER_RECORD * sizeof(uint32_t)))

typedef enum { SAVE_IDLE, SAVE_OK, SAVE_FULL, SAVE_ERROR } SaveStatus;

static volatile uint32_t current_period_ms, next_slot;
static volatile SaveStatus save_status;

void SysTick_Handler(void) { HAL_IncTick(); }

static SaveStatus append_period(size_t slot, uint32_t period)
{
  // 1. 只有全新的槽才能追加。空间用尽后保留旧参数。
  if (slot >= SLOT_COUNT)
  {
    return SAVE_FULL;
  }
  if (HAL_FLASH_Unlock() != HAL_OK)
  {
    return SAVE_ERROR;
  }

  // 2. 先写参数与校验值。任意一步失败后直接上锁，不发布提交标记。
  const uint32_t address = SECTOR_ADDRESS + slot * WORDS_PER_RECORD * sizeof(uint32_t);
  HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, period);
  if (status == HAL_OK)
  {
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + 4U, settings_crc(period));
  }

  // 3. 最后写提交标记。复位后的读者还会重新核对 CRC。
  if (status == HAL_OK)
  {
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + 8U, SETTINGS_COMMIT);
  }
  const HAL_StatusTypeDef lock_status = HAL_FLASH_Lock();
  return status == HAL_OK && lock_status == HAL_OK ? SAVE_OK : SAVE_ERROR;
}

int main(void)
{
  // 1. 每次启动从专用扇区恢复参数。全空时使用 1000 ms。
  HAL_Init();
  // 配置区会在运行时改写，读回不经过 Flash 数据缓存。
  __HAL_FLASH_DATA_CACHE_DISABLE();
  __HAL_FLASH_DATA_CACHE_RESET();
  const volatile uint32_t *const words = (const volatile uint32_t *)SECTOR_ADDRESS;
  JournalState settings = journal_scan(words, SLOT_COUNT);
  current_period_ms = settings.period_ms;
  next_slot = settings.next_slot;
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef key = {.Pin = GPIO_PIN_0, .Mode = GPIO_MODE_INPUT};
  GPIO_InitTypeDef led = {.Pin = GPIO_PIN_2, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOA, &key);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  HAL_GPIO_Init(GPIOB, &led);
  Debounce button = {0};
  uint32_t last = HAL_GetTick();

  for (;;)
  {
    // 2. 按键稳定后请求一条新记录。随后按启动时相同的规则读回实际可用值。
    const uint32_t now = HAL_GetTick();
    const bool raw = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_SET;
    if (debounce_update(&button, raw, now, 20U) == KEY_PRESSED)
    {
      const uint32_t updated = settings.period_ms > PERIOD_MAX_MS - PERIOD_MIN_MS ? PERIOD_MIN_MS :
                               settings.period_ms + PERIOD_MIN_MS;
      save_status = append_period(settings.next_slot, updated);
      settings = journal_scan(words, SLOT_COUNT);
      current_period_ms = settings.period_ms;
      next_slot = settings.next_slot;
    }

    // 3. LED 使用恢复或读回的参数。掉电留下半条记录时仍按上一条有效记录闪动。
    if ((uint32_t)(now - last) >= settings.period_ms)
    {
      last = now;
      HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_2);
    }
    __WFI();
  }
}
