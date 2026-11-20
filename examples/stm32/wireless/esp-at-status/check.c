#include "at_reply.h"
#include <assert.h>
#include <stdio.h>

static AtReply feed_text(AtParser *parser, const char *text)
{
  AtReply status = AT_MORE;
  for (; *text; ++text)
  {
    assert(status == AT_MORE);
    status = at_feed(parser, (uint8_t)*text);
  }
  return status;
}

int main(void)
{
  AtParser parser = {.wifi_state = -1};
  assert(feed_text(&parser, "AT+CWSTATE?\r\nWIFI CONNECTED\r\n+CWST") == AT_MORE);
  assert(parser.wifi_state == -1);
  assert(feed_text(&parser, "ATE:2,\"lab\"\r\n") == AT_MORE);
  assert(parser.wifi_state == 2);
  assert(feed_text(&parser, "OK\r\n") == AT_OK);
  assert(feed_text(&parser, "+CWSTATE:9,\"lab\"\r\n") == AT_MORE && parser.wifi_state == 2);
  assert(feed_text(&parser, "busy p...\r\n") == AT_BUSY);
  assert(feed_text(&parser, "ERROR\r\n") == AT_ERROR);

  for (unsigned i = 0; i < 200U; ++i) { assert(at_feed(&parser, 'X') == AT_MORE); }
  assert(feed_text(&parser, "OK\r\n") == AT_MORE);
  assert(feed_text(&parser, "OK\r\n") == AT_OK);
  assert(at_feed(&parser, 'O') == AT_MORE);
  assert(at_feed(&parser, 0U) == AT_MORE);
  assert(feed_text(&parser, "K\r\n") == AT_MORE);
  puts("ESP-AT: split status, unsolicited event, OK, error, busy and discarded line");
}
