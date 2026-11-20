#ifndef KB_ESP_AT_REPLY_H
#define KB_ESP_AT_REPLY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef enum { AT_MORE, AT_OK, AT_ERROR, AT_BUSY } AtReply;
typedef struct {
  char line[128];
  size_t used;
  bool discard;
  int wifi_state;
} AtParser;

// 输入一字节。只有一行完整的终止回复才交给事务调用者。
static inline AtReply at_feed(AtParser *parser, uint8_t byte)
{
  // 1. 收到行尾前积累内容。超长或含 NUL 的行一直丢到行尾。
  if (byte == '\r') { return AT_MORE; }
  if (byte != '\n')
  {
    if (byte == 0U || parser->used == sizeof(parser->line) - 1U) { parser->discard = true; }
    if (!parser->discard) { parser->line[parser->used++] = (char)byte; }
    return AT_MORE;
  }

  // 2. 行尾释放缓冲区。丢弃的半行和其他主动通知都不能完成命令。
  if (parser->discard)
  {
    parser->used = 0U;
    parser->discard = false;
    return AT_MORE;
  }
  parser->line[parser->used] = '\0';
  parser->used = 0U;
  if (strcmp(parser->line, "OK") == 0) { return AT_OK; }
  if (strcmp(parser->line, "ERROR") == 0) { return AT_ERROR; }
  if (strncmp(parser->line, "busy ", 5U) == 0) { return AT_BUSY; }

  // 3. 查询结果是中间行。调用者等到 OK 后才使用该状态。
  const char prefix[] = "+CWSTATE:";
  const size_t state_index = sizeof(prefix) - 1U;
  if (strncmp(parser->line, prefix, state_index) == 0 &&
      parser->line[state_index] >= '0' && parser->line[state_index] <= '4' &&
      parser->line[state_index + 1U] == ',')
  {
    parser->wifi_state = parser->line[state_index] - '0';
  }
  return AT_MORE;
}

#endif
