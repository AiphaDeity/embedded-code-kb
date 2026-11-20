#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

static uint32_t field_mask(unsigned shift, unsigned width)
{
  // 1. 位编号从 0 起。先判断 shift，再计算剩余位数，避免无符号下溢。
  if (shift >= 32U || width == 0U || width > 32U - shift) { return 0U; }

  // 2. 32 位字段没有同宽的移位操作，直接返回全 1。
  return width == 32U ? UINT32_MAX : (UINT32_C(1) << width) - 1U;
}

// 成功时把字段右对齐到 out。参数无效时返回 false，保持 out 原值。
static bool read_field(uint32_t word, unsigned shift, unsigned width, uint32_t *out)
{
  const uint32_t mask = field_mask(shift, width);
  if (out == NULL || mask == 0U) { return false; }
  *out = (word >> shift) & mask;
  return true;
}

// 成功时给出新字，其他位保留。新值放不下时拒绝写入，out 保持原值。
static bool change_field(uint32_t word, unsigned shift, unsigned width,
                         uint32_t value, uint32_t *out)
{
  // 1. 先验证字段范围与数值。只要有一位落到 mask 外就拒绝。
  const uint32_t mask = field_mask(shift, width);
  if (out == NULL || mask == 0U || (value & ~mask) != 0U) { return false; }

  // 2. 将掩码和新值左移到字段位置。先清旧字段，再合并新值。
  *out = (word & ~(mask << shift)) | (value << shift);
  return true;
}

int main(void)
{
  // 1. 在内存里的示例字修改 bit[6:4]，再读出字段核对。
  const uint32_t initial = UINT32_C(0xA5A500F3);
  uint32_t word = initial;
  uint32_t field = 0U;
  assert(change_field(word, 4U, 3U, 5U, &word));
  assert(read_field(word, 4U, 3U, &field) && field == 5U);
  assert(word == UINT32_C(0xA5A500D3));
  printf("0x%08" PRIX32 " -> 0x%08" PRIX32 ", field=%" PRIu32 "\n",
         initial, word, field);

  // 2. 越界与过宽值都保留旧值。最后核对整字字段和最高单个位。
  assert(!change_field(word, 4U, 3U, 8U, &word));
  assert(!change_field(word, 31U, 2U, 0U, &word));
  assert(!change_field(word, 0U, 0U, 0U, &word));
  assert(!read_field(word, 32U, 1U, &field) && field == 5U);
  assert(word == UINT32_C(0xA5A500D3));
  assert(change_field(word, 0U, 32U, UINT32_MAX, &word) && word == UINT32_MAX);
  assert(change_field(word, 31U, 1U, 0U, &word) && word == UINT32_C(0x7FFFFFFF));
  puts("bit field: bounds, full width and unrelated bits preserved");
}
