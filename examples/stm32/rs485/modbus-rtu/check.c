#include <assert.h>
#include <stdio.h>
#include <string.h>
#include "rtu.h"

int main(void)
{
  const uint16_t holding[2] = {0x1234U, 0xABCDU};
  RtuRx rx = {0};
  RtuFrame request, response = {0};
  uint8_t tx[8];
  build_read_request(tx, 0U, 2U);
  const uint8_t expected[8] = {1U, 3U, 0U, 0U, 0U, 2U, 0xC4U, 0x0BU};
  assert(memcmp(tx, expected, sizeof(tx)) == 0);

  // 1. 重放连续请求并跨 32 位微秒计数回绕。定界后服务端返回两个值。
  uint32_t time = UINT32_MAX - 3000U;
  for (size_t i = 0; i < sizeof(tx); ++i, time += 1146U)
  {
    assert(!rtu_receive(&rx, time, tx[i], false, &request));
  }
  assert(!rtu_finish(&rx, rx.last_us + RX_FRAME_US - 1U, &request));
  assert(rtu_finish(&rx, rx.last_us + RX_FRAME_US, &request));
  response.count = build_server_reply(&request, holding, response.data);
  assert(response.count == 9U);
  uint16_t values[2] = {0};
  uint8_t exception = 0U;
  assert(read_client_reply(&response, values, &exception) == REPLY_VALUES);
  assert(values[0] == holding[0] && values[1] == holding[1]);
  RtuFrame damaged = response;
  damaged.data[3] ^= 0x01U;
  assert(read_client_reply(&damaged, values, &exception) == REPLY_IGNORED);
  assert(values[0] == holding[0] && values[1] == holding[1] && exception == 0U);

  // 2. 地址越界和数量为零分别得到异常 02 和 03，CRC 错误静默丢弃。
  build_read_request(request.data, 1U, 2U);
  response.count = build_server_reply(&request, holding, response.data);
  assert(read_client_reply(&response, values, &exception) == REPLY_EXCEPTION && exception == 2U);
  build_read_request(request.data, 0U, 0U);
  response.count = build_server_reply(&request, holding, response.data);
  assert(read_client_reply(&response, values, &exception) == REPLY_EXCEPTION && exception == 3U);
  request.data[7] ^= 1U;
  assert(build_server_reply(&request, holding, response.data) == 0U);

  // 3. 逐字节测试过大的帧内间隔及容量用尽。都等到帧边界才交出坏帧。
  assert(!rtu_receive(&rx, 0U, 1U, false, &request));
  assert(!rtu_receive(&rx, RX_INTERCHAR_US, 3U, false, &request));
  assert(rtu_finish(&rx, RX_INTERCHAR_US + RX_FRAME_US, &request) && request.bad);
  for (unsigned i = 0; i <= RTU_LIMIT; ++i)
  {
    assert(!rtu_receive(&rx, 1146U * i, 0U, false, &request));
  }
  assert(rtu_finish(&rx, rx.last_us + RX_FRAME_US, &request) && request.bad);
  puts("Modbus RTU: exact wire request, frame gap and wrap, reply, exceptions, CRC and overflow");
  return 0;
}
