#ifndef SKY_NOR_STORE_H
#define SKY_NOR_STORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "../../../c/common/crc32.h"

enum { NOR_SECTOR = 4096, NOR_BODY = 12, NOR_RECORD = 16 };
#define NOR_SLOT_A 0xFFE000U
#define NOR_SLOT_B 0xFFF000U
#define NOR_COMMIT 0x4B425341U

typedef struct
{
  void *context;
  bool (*read)(void *context, uint32_t address, uint8_t *data, size_t count);
  bool (*program)(void *context, uint32_t address, const uint8_t *data, size_t count);
  bool (*erase_sector)(void *context, uint32_t address);
} NorIo;

typedef struct
{
  uint32_t generation, period_ms, address;
  bool present;
} NorSetting;

static inline uint32_t nor_get_u32(const uint8_t *p)
{
  return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

static inline void nor_put_u32(uint8_t *p, uint32_t v)
{
  for (unsigned i = 0; i < 4U; ++i)
  {
    p[i] = (uint8_t)(v >> (8U * i));
  }
}

static inline void encode_setting(uint8_t record[NOR_RECORD], uint32_t generation, uint32_t period)
{
  nor_put_u32(record, generation);
  nor_put_u32(record + 4U, period);
  nor_put_u32(record + 8U, kb_crc32(record, 8U));
  nor_put_u32(record + 12U, NOR_COMMIT);
}

static inline bool read_slot(const NorIo *io, uint32_t address, NorSetting *setting)
{
  uint8_t bytes[NOR_RECORD];
  if (!io->read(io->context, address, bytes, sizeof(bytes)))
  {
    return false;
  }
  setting->generation = nor_get_u32(bytes);
  setting->period_ms = nor_get_u32(bytes + 4U);
  setting->address = address;
  setting->present = nor_get_u32(bytes + 12U) == NOR_COMMIT &&
                     setting->period_ms >= 100U && setting->period_ms <= 1000U &&
                     nor_get_u32(bytes + 8U) == kb_crc32(bytes, 8U);
  return true;
}

// 任一槽读失败时保持输出不变，避免把未读到的较新记录误判为可擦除。
static inline bool nor_load(const NorIo *io, NorSetting *result)
{
  // 1. A、B 都读取后，才在有效记录中选较新的代数。
  NorSetting a, b;
  if (!read_slot(io, NOR_SLOT_A, &a) || !read_slot(io, NOR_SLOT_B, &b))
  {
    return false;
  }
  const uint32_t difference = b.generation - a.generation;
  if (b.present && (!a.present || (difference != 0U && difference < 0x80000000U)))
  {
    *result = b;
  }
  else
  {
    *result = a.present ? a : (NorSetting){.period_ms = 1000U};
  }
  return true;
}

// 保存成功后更新 result。失败时保留调用方已有值，下次可重新加载介质。
static inline bool nor_save(const NorIo *io, uint32_t period, NorSetting *result)
{
  NorSetting latest;
  if (period < 100U || period > 1000U || !nor_load(io, &latest))
  {
    return false;
  }

  // 1. 正在使用的槽保持原样，目标是另一扇区。无记录时从 A 开始。
  const uint32_t address = latest.address == NOR_SLOT_A ? NOR_SLOT_B : NOR_SLOT_A;
  uint8_t wanted[NOR_RECORD], readback[NOR_RECORD];
  encode_setting(wanted, latest.generation + 1U, period);
  if (!io->erase_sector(io->context, address) ||
      !io->read(io->context, address, readback, sizeof(readback)))
  {
    return false;
  }
  for (size_t i = 0; i < sizeof(readback); ++i)
  {
    if (readback[i] != 0xFFU) { return false; }
  }

  // 2. 先写主体并比对，确认内容正确后才单独写提交字。
  if (!io->program(io->context, address, wanted, NOR_BODY) ||
      !io->read(io->context, address, readback, NOR_BODY) ||
      memcmp(wanted, readback, NOR_BODY) != 0 ||
      !io->program(io->context, address + NOR_BODY, wanted + NOR_BODY, NOR_RECORD - NOR_BODY))
  {
    return false;
  }

  // 3. 读回完整记录并确认激活候选，新的参数此时才交给调用者。
  NorSetting committed;
  if (!read_slot(io, address, &committed) || !committed.present ||
      committed.generation != latest.generation + 1U || committed.period_ms != period)
  {
    return false;
  }
  *result = committed;
  return true;
}
#endif
