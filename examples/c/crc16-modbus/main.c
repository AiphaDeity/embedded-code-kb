#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "crc16.h"

int main(void)
{
  // 1. 先用标准检查向量核对参数，再将 CRC 写入一条读保持寄存器的请求。
  const uint8_t check[] = {'1', '2', '3', '4', '5', '6', '7', '8', '9'};
  assert(crc16_modbus(check, sizeof(check)) == 0x4B37U);
  uint8_t request[8] = {0x01U, 0x03U, 0x00U, 0x00U, 0x00U, 0x0AU, 0x77U, 0x88U};
  assert(!append_crc(request, 7U, 6U));
  assert(!append_crc(request, sizeof(request), SIZE_MAX));
  assert(request[6] == 0x77U && request[7] == 0x88U);
  assert(append_crc(request, sizeof(request), 6U));
  assert(request[6] == 0xC5U && request[7] == 0xCDU);

  // 2. 收到相同完整帧时校验通过。改动负载或缩短到不足 CRC 长度时拒绝。
  assert(valid_crc(request, sizeof(request)));
  request[2] ^= 0x01U;
  assert(!valid_crc(request, sizeof(request)));
  assert(!valid_crc(request, 1U));
  puts("CRC-16/MODBUS: 4B37, request tail C5 CD");
  return 0;
}
