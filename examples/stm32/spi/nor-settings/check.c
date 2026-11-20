#include <assert.h>
#include <stdio.h>
#include "store.h"

typedef struct
{
  uint8_t flash[2U * NOR_SECTOR];
  size_t bytes_left;
  bool unreadable_b;
  size_t erases;
} Mock;

static bool read_flash(void *context, uint32_t address, uint8_t *data, size_t count)
{
  Mock *mock = context;
  if (mock->unreadable_b && address == NOR_SLOT_B) { return false; }
  memcpy(data, &mock->flash[address - NOR_SLOT_A], count);
  return true;
}

static bool erase_flash(void *context, uint32_t address)
{
  Mock *mock = context;
  memset(&mock->flash[address - NOR_SLOT_A], 0xFF, NOR_SECTOR);
  ++mock->erases;
  return true;
}

static bool program_flash(void *context, uint32_t address, const uint8_t *data, size_t count)
{
  Mock *mock = context;
  uint8_t *target = &mock->flash[address - NOR_SLOT_A];
  for (size_t i = 0; i < count; ++i)
  {
    if (mock->bytes_left == 0U) { return false; }
    assert((target[i] & data[i]) == data[i]);
    target[i] &= data[i];
    --mock->bytes_left;
  }
  return true;
}

int main(void)
{
  Mock mock = {.bytes_left = SIZE_MAX};
  memset(mock.flash, 0xFF, sizeof(mock.flash));
  const NorIo io = {.context = &mock, .read = read_flash, .program = program_flash, .erase_sector = erase_flash};
  NorSetting setting;
  assert(nor_load(&io, &setting) && !setting.present && setting.period_ms == 1000U);
  assert(nor_save(&io, 400U, &setting) && setting.address == NOR_SLOT_A);
  assert(nor_save(&io, 500U, &setting) && setting.address == NOR_SLOT_B);
  const Mock saved = mock;

  // 1. 在新槽任意一个字节之前模拟掉电。恢复始终可取到原来的 B。
  for (size_t cut = 0; cut < NOR_RECORD; ++cut)
  {
    mock = saved;
    mock.bytes_left = cut;
    assert(!nor_save(&io, 600U, &setting));
    assert(nor_load(&io, &setting) && setting.period_ms == 500U && setting.address == NOR_SLOT_B);
  }

  // 2. 最新槽校验损坏时回退。读取另一槽失败时禁止擦除。
  mock = saved;
  assert(nor_save(&io, 600U, &setting) && setting.address == NOR_SLOT_A);
  mock.flash[8] ^= 1U;
  assert(nor_load(&io, &setting) && setting.period_ms == 500U);
  const size_t before = mock.erases;
  mock.unreadable_b = true;
  assert(!nor_save(&io, 700U, &setting) && mock.erases == before);

  // 3. 旧代数最大，新代数回绕到 0。模差仍选择新槽。
  mock = saved;
  encode_setting(&mock.flash[0], UINT32_MAX - 1U, 400U);
  encode_setting(&mock.flash[NOR_SECTOR], UINT32_MAX, 500U);
  assert(nor_save(&io, 600U, &setting) && setting.generation == 0U);
  assert(nor_load(&io, &setting) && setting.period_ms == 600U);
  puts("SPI NOR: interrupted writes, corrupt CRC, unreadable slot and generation wrap");
  return 0;
}
