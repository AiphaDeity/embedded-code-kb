#ifndef SKY_ROLLBACK_FLASH_H
#define SKY_ROLLBACK_FLASH_H

#include "stm32f4xx_hal.h"
#include "layout.h"
#include "policy.h"

#define JOURNAL_RECORDS (JOURNAL_BYTES / (STATE_WORDS * sizeof(uint32_t)))

static inline void state_flash_init(void)
{
  // 扫描与立即读回都直接读取实际 Flash 字，不经过数据缓存。
  __HAL_FLASH_DATA_CACHE_DISABLE();
  __HAL_FLASH_DATA_CACHE_RESET();
}

static inline BootJournal load_boot_state(void)
{
  return scan_boot_state((const volatile uint32_t *)JOURNAL_ADDR, JOURNAL_RECORDS);
}

// 成功表示元数据写完并通过读回。永远不在此路径擦除扇区。
static inline bool persist_boot_state(BootJournal snapshot, BootState state)
{
  // 1. 只追加到确认还未使用的槽。空间用尽或 Flash 解锁失败就保持旧状态。
  if (snapshot.next_record >= JOURNAL_RECORDS || HAL_FLASH_Unlock() != HAL_OK)
  {
    return false;
  }
  const uint32_t offset = (uint32_t)snapshot.next_record * STATE_WORDS * sizeof(uint32_t);
  const uint32_t address = JOURNAL_ADDR + offset;
  HAL_StatusTypeDef status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address, (uint32_t)state);
  if (status == HAL_OK)
  {
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + 4U, boot_state_crc(state));
  }

  // 2. 状态与 CRC 都写完才落下提交标记。每个出口都让 Flash 重新上锁。
  if (status == HAL_OK)
  {
    status = HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, address + 8U, STATE_COMMIT);
  }
  const HAL_StatusTypeDef locked = HAL_FLASH_Lock();
  const BootJournal verified = load_boot_state();
  return status == HAL_OK && locked == HAL_OK && verified.state == state &&
         verified.next_record == snapshot.next_record + 1U;
}

#endif
