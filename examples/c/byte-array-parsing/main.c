#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// 从 data 的 offset 字节处读取大端无符号 16 位整数。
// length 为可读字节数。成功时写入 *out 并返回 true。
// 指针为空或剩余字节不足时返回 false，已有输出值保持原值。
static bool read_u16_be(const uint8_t *data, size_t length, size_t offset, uint16_t *out)
{
  // 1. 确认输入和输出指针都有效，再检查报文里是否有足够的字节。
  if (data == NULL || out == NULL)
  {
    return false;
  }

  // 2. 先检查偏移再做减法，避免无符号长度回绕后误判为仍有数据。
  if (offset > length || length - offset < 2U)
  {
    return false;
  }

  // 3. 大端字段的第一个字节占高八位。检查全部通过后才写回结果。
  *out = (uint16_t)(((uint16_t)data[offset] << 8) | data[offset + 1U]);
  return true;
}

int main(void)
{
  const uint8_t data[] = {0x11, 0x22, 0x33};
  const size_t offsets[] = {0, 1, 2, 3, SIZE_MAX};
  const uint16_t expected_values[] = {0x1122, 0x2233};
  uint16_t value = 0xCAFE;

  // 1. main 用同一份报文依次试五个偏移，value 保留上轮结果，用来观察读取失败时发生了什么。
  for (size_t i = 0; i < sizeof(offsets) / sizeof(offsets[0]); ++i)
  {
    // 2. 调用前留下旧值，再把本轮偏移交给解析函数。打印的是函数返回后调用者实际拿到的值。
    const uint16_t previous = value;
    const bool ok = read_u16_be(data, sizeof(data), offsets[i], &value);
    printf("offset=%zu  %s  value=0x%04" PRIX16 "\n",
           offsets[i], ok ? "ok" : "rejected", value);

    // 3. 前两次偏移都有两个可读字节，调用者同时核对成功标志和解析值，任一不符就停止测试。
    if (i < sizeof(expected_values) / sizeof(expected_values[0]))
    {
      if (!ok || value != expected_values[i])
      {
        return EXIT_FAILURE;
      }
    }
    // 后三次都应被拒绝。如果误报成功或覆盖了旧值，向运行示例的人返回失败状态。
    else if (ok || value != previous)
    {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
