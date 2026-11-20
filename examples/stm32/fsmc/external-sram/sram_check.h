#ifndef KB_SRAM_CHECK_H
#define KB_SRAM_CHECK_H

#include <stdint.h>

enum { SRAM_WORDS = 512U * 1024U };

typedef enum { SRAM_PASS, SRAM_DATA_BITS, SRAM_ADDRESS_ALIAS, SRAM_CONTENT } SramStage;
typedef struct
{
  SramStage stage;
  uint32_t word;
  uint16_t expected, actual;
} SramResult;

typedef struct
{
  void *context;
  uint16_t (*read)(void *context, uint32_t word);
  void (*write)(void *context, uint32_t word, uint16_t value);
} SramIo;

static inline SramResult expect_word(const SramIo *io, SramStage stage,
                                    uint32_t word, uint16_t expected)
{
  const uint16_t actual = io->read(io->context, word);
  return (SramResult){actual == expected ? SRAM_PASS : stage, word, expected, actual};
}

static inline uint16_t word_pattern(uint32_t word)
{
  return (uint16_t)((word * 73U) ^ (word >> 16U) ^ 0xA55AU);
}

// 传入 512K 个可读写半字。本实验会覆盖整片测试区，发现首个差异立即返回。
static inline SramResult sram_check(const SramIo *io)
{
  // 1. 固定在首地址，依次翻转 16 条数据线，读回正向与反向图样。
  for (unsigned bit = 0U; bit < 16U; ++bit)
  {
    const uint16_t one = (uint16_t)(1U << bit);
    const uint16_t patterns[] = {one, (uint16_t)~one};
    for (unsigned i = 0U; i < 2U; ++i)
    {
      io->write(io->context, 0U, patterns[i]);
      const SramResult result = expect_word(io, SRAM_DATA_BITS, 0U, patterns[i]);
      if (result.stage != SRAM_PASS) { return result; }
    }
  }

  // 2. 所有单地址位的探测位置先写相同值。每次翻转一项，确认其他位置仍保持。
  const uint16_t same = 0xAAAAU;
  const uint16_t changed = 0x5555U;
  io->write(io->context, 0U, same);
  for (uint32_t bit = 1U; bit < SRAM_WORDS; bit <<= 1U)
  {
    io->write(io->context, bit, same);
  }
  for (uint32_t word = 1U; word < SRAM_WORDS; word <<= 1U)
  {
    io->write(io->context, word, changed);
    SramResult result = expect_word(io, SRAM_ADDRESS_ALIAS, 0U, same);
    if (result.stage != SRAM_PASS) { return result; }
    for (uint32_t other = 1U; other < SRAM_WORDS; other <<= 1U)
    {
      const uint16_t expected = other == word ? changed : same;
      result = expect_word(io, SRAM_ADDRESS_ALIAS, other, expected);
      if (result.stage != SRAM_PASS) { return result; }
    }
    io->write(io->context, word, same);
  }

  // 3. 每个半字先写入与地址相关的图样，再独立读完整片，检测跨区域的覆盖。
  for (uint32_t word = 0U; word < SRAM_WORDS; ++word)
  {
    io->write(io->context, word, word_pattern(word));
  }
  for (uint32_t word = 0U; word < SRAM_WORDS; ++word)
  {
    const SramResult result = expect_word(io, SRAM_CONTENT, word, word_pattern(word));
    if (result.stage != SRAM_PASS) { return result; }
  }
  return (SramResult){.stage = SRAM_PASS};
}

#endif
