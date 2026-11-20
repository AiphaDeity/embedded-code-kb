#ifndef SKY_SETTINGS_JOURNAL_H
#define SKY_SETTINGS_JOURNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../../../c/common/crc32.h"

enum { PERIOD_MIN_MS = 100, PERIOD_MAX_MS = 1000, WORDS_PER_RECORD = 3 };
#define SETTINGS_COMMIT 0x4B421AA5U

typedef struct
{
  uint32_t period_ms;
  size_t next_slot;
  bool found;
} JournalState;

static inline uint32_t settings_crc(uint32_t value)
{
  const uint8_t bytes[4] = {(uint8_t)value, (uint8_t)(value >> 8),
                            (uint8_t)(value >> 16), (uint8_t)(value >> 24)};
  return kb_crc32(bytes, sizeof(bytes));
}

// 每个槽占 3 个字。按物理顺序扫描，选择最后一条完整记录和最后一个用过的槽之后的位置。
static inline JournalState journal_scan(const volatile uint32_t *words, size_t slots)
{
  JournalState result = {.period_ms = PERIOD_MAX_MS};
  for (size_t i = 0; i < slots; ++i)
  {
    // 1. 只要任何字已写入，槽就已使用。后续追加不覆盖它。
    const volatile uint32_t *const record = words + i * WORDS_PER_RECORD;
    const uint32_t value = record[0], crc = record[1], commit = record[2];
    if (value != UINT32_MAX || crc != UINT32_MAX || commit != UINT32_MAX)
    {
      result.next_slot = i + 1U;
    }

    // 2. 标记、范围和 CRC 都通过时，该槽替换先前有效记录。
    if (commit == SETTINGS_COMMIT && value >= PERIOD_MIN_MS && value <= PERIOD_MAX_MS &&
        crc == settings_crc(value))
    {
      result.period_ms = value;
      result.found = true;
    }
  }
  return result;
}

#endif
