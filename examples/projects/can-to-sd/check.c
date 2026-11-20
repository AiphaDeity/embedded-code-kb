#include <assert.h>
#include <string.h>
#include "can_log.h"

int main(void)
{
  FrameQueue queue = {0};
  CanRecord report = {0};
  assert(!frame_pop(&queue, &report));
  // 1. 追加到满后丢新帧，再交替入队与出队，验证顺序跨过回绕。
  for (uint32_t i = 0; i < 15U; ++i)
  {
    assert(frame_push(&queue, (CanRecord){.id = i}));
  }
  assert(!frame_push(&queue, (CanRecord){.id = 15U}));
  for (uint32_t i = 0; i < 30U; ++i)
  {
    assert(frame_pop(&queue, &report) && report.id == i);
    assert(frame_push(&queue, (CanRecord){.id = i + 15U}));
  }

  // 2. 比对标准帧数据和扩展远程帧。远程帧保留 DLC，数据列为空。
  char row[MAX_CSV_LINE];
  report = (CanRecord){.time_ms = 123U, .id = 0x321U, .length = 2U, .data = {0x00U, 0xABU}};
  assert(format_can_record(&report, row, sizeof(row)) == strlen("123,S,00000321,D,2,00AB\r\n"));
  assert(strcmp(row, "123,S,00000321,D,2,00AB\r\n") == 0);
  report = (CanRecord){.time_ms = 42U, .id = 0x1FFFFFFFU, .extended = true, .remote = true, .length = 8U};
  assert(format_can_record(&report, row, sizeof(row)) > 0U);
  assert(strcmp(row, "42,E,1FFFFFFF,R,8,\r\n") == 0);
  assert(format_can_record(&report, row, 4U) == 0U);
  report.length = 9U;
  assert(format_can_record(&report, row, sizeof(row)) == 0U);
  puts("CAN logger: queue full and wrap, data and remote CSV, invalid length");
  return 0;
}
