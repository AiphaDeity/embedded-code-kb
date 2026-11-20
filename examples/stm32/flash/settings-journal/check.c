#include <assert.h>
#include <stdio.h>
#include "journal.h"

int main(void)
{
  uint32_t words[6U * WORDS_PER_RECORD];
  for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); ++i)
  {
    words[i] = UINT32_MAX;
  }

  // 1. 空扇区选默认值。先模拟旧记录已提交，再模拟新的写到一半时掉电。
  JournalState state = journal_scan(words, 6U);
  assert(!state.found && state.period_ms == 1000U && state.next_slot == 0U);
  words[0] = 400U;
  words[1] = settings_crc(words[0]);
  words[2] = SETTINGS_COMMIT;
  words[3] = 500U;
  words[4] = settings_crc(words[3]);
  state = journal_scan(words, 6U);
  assert(state.found && state.period_ms == 400U && state.next_slot == 2U);

  // 2. 后续追加到新槽。再制造标记存在但 CRC 错误的记录。
  words[6] = 600U;
  words[7] = settings_crc(words[6]);
  words[8] = SETTINGS_COMMIT;
  words[9] = 700U;
  words[10] = settings_crc(800U);
  words[11] = SETTINGS_COMMIT;
  state = journal_scan(words, 6U);
  assert(state.period_ms == 600U && state.next_slot == 4U);

  // 3. 若最后一个槽是脏的，追加位置等于槽数量，板端据此报告满。
  words[16] = 1U;
  state = journal_scan(words, 6U);
  assert(state.period_ms == 600U && state.next_slot == 6U);
  puts("flash journal: erased, torn, bad CRC, latest valid and full");
  return 0;
}
