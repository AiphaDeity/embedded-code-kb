#include "sram_check.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

static uint16_t storage[SRAM_WORDS];

typedef struct
{
  uint32_t word_mask;
  uint16_t stuck_low;
  bool corrupt_word_seven;
} Model;

static uint16_t read_word(void *context, uint32_t word)
{
  const Model *model = context;
  uint16_t value = storage[word & model->word_mask];
  return model->corrupt_word_seven && word == 7U ? (uint16_t)(value ^ 1U) : value;
}

static void write_word(void *context, uint32_t word, uint16_t value)
{
  const Model *model = context;
  storage[word & model->word_mask] = value & (uint16_t)~model->stuck_low;
}

int main(void)
{
  Model model = {.word_mask = SRAM_WORDS - 1U};
  const SramIo io = {.context = &model, .read = read_word, .write = write_word};
  const SramResult pass = sram_check(&io);
  assert(pass.stage == SRAM_PASS);
  assert(storage[SRAM_WORDS - 1U] == word_pattern(SRAM_WORDS - 1U));

  model.stuck_low = 0x0008U;
  const SramResult data = sram_check(&io);
  assert(data.stage == SRAM_DATA_BITS && data.word == 0U && data.expected != data.actual);

  model.stuck_low = 0U;
  model.word_mask &= ~(1U << 16U);
  const SramResult alias = sram_check(&io);
  assert(alias.stage == SRAM_ADDRESS_ALIAS && alias.expected != alias.actual);

  model.word_mask = SRAM_WORDS - 1U;
  model.corrupt_word_seven = true;
  const SramResult data_error = sram_check(&io);
  assert(data_error.stage == SRAM_CONTENT && data_error.word == 7U);
  assert(data_error.expected == word_pattern(7U) && data_error.actual == (word_pattern(7U) ^ 1U));
  puts("SRAM: full-range pattern, stuck data bit, A16 alias, isolated read corruption");
}
