#ifndef SKY_ROLLBACK_POLICY_H
#define SKY_ROLLBACK_POLICY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "../../c/common/crc32.h"

typedef enum { SLOT_NONE, SLOT_A, SLOT_B } ImageSlot;
typedef enum
{
  CONFIRMED_A = 1, REQUEST_B, TRIED_B, CONFIRMED_B, REQUEST_A, TRIED_A,
} BootState;

enum { STATE_WORDS = 3 };
#define STATE_COMMIT 0x4B42544CU

typedef struct
{
  BootState state;
  size_t next_record;
} BootJournal;

typedef struct
{
  ImageSlot target;
  ImageSlot fallback;
  BootState save_before_jump;
  bool trial;
} BootPlan;

static inline uint32_t boot_state_crc(uint32_t value)
{
  const uint8_t encoded[4] = {(uint8_t)value, (uint8_t)(value >> 8),
                              (uint8_t)(value >> 16), (uint8_t)(value >> 24)};
  return kb_crc32(encoded, sizeof(encoded));
}

// 每条记录 3 个字。跳过不完整或损坏项，追加位置在最后一个已使用槽之后。
static inline BootJournal scan_boot_state(const volatile uint32_t *words, size_t records)
{
  BootJournal snapshot = {.state = CONFIRMED_A};
  for (size_t slot = 0; slot < records; ++slot)
  {
    const volatile uint32_t *record = words + slot * STATE_WORDS;
    const uint32_t state = record[0], crc = record[1], commit = record[2];
    if (state != UINT32_MAX || crc != UINT32_MAX || commit != UINT32_MAX)
    {
      snapshot.next_record = slot + 1U;
    }
    if (commit == STATE_COMMIT && state >= CONFIRMED_A && state <= TRIED_A &&
        crc == boot_state_crc(state))
    {
      snapshot.state = (BootState)state;
    }
  }
  return snapshot;
}

static inline ImageSlot confirmed_slot(BootState state)
{
  return state == CONFIRMED_A || state == REQUEST_B || state == TRIED_B ? SLOT_A : SLOT_B;
}

static inline BootState confirmed_state(ImageSlot slot)
{
  return slot == SLOT_A ? CONFIRMED_A : CONFIRMED_B;
}

static inline bool is_trial(BootState state, ImageSlot slot)
{
  return (state == TRIED_A && slot == SLOT_A) || (state == TRIED_B && slot == SLOT_B);
}

// valid_a/valid_b 是本次复位后独立校验的结果。目标为 SLOT_NONE 时留在 Boot。
static inline BootPlan plan_boot(BootState state, bool valid_a, bool valid_b)
{
  const ImageSlot safe = confirmed_slot(state);
  const ImageSlot candidate = safe == SLOT_A ? SLOT_B : SLOT_A;
  const bool safe_valid = safe == SLOT_A ? valid_a : valid_b;
  const bool candidate_valid = candidate == SLOT_A ? valid_a : valid_b;
  BootPlan plan = {.save_before_jump = state};
  if (!safe_valid)
  {
    return plan;
  }
  plan.target = plan.fallback = safe;

  // 1. 只有已请求且两边都可用时允许试跑。TRIED 表示机会已经用过。
  if ((state == REQUEST_A || state == REQUEST_B) && candidate_valid)
  {
    plan.target = candidate;
    plan.save_before_jump = candidate == SLOT_A ? TRIED_A : TRIED_B;
    plan.trial = true;
  }
  else
  {
    // 2. 失败候选或试跑后重启，都恢复成此前确认的状态。
    plan.save_before_jump = confirmed_state(safe);
  }
  return plan;
}

// 试跑标记必须先持久化。写失败只允许回到已确认槽。
static inline ImageSlot chosen_slot(BootPlan plan, bool persisted)
{
  return plan.trial && !persisted ? plan.fallback : plan.target;
}
#endif
