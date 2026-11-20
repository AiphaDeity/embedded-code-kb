#ifndef KB_CRC16_MODBUS_H
#define KB_CRC16_MODBUS_H
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// 返回 CRC-16/MODBUS。length > 0 时，data 至少指向 length 字节。
static inline uint16_t crc16_modbus(const uint8_t *data, size_t length)
{
  uint16_t crc = 0xFFFFU;
  for (size_t i = 0; i < length; ++i)
  {
    // 1. 将下一个字节合进低八位，从最低位开始处理。
    crc ^= data[i];

    // 2. 移出的位为 1 时用反射后的多项式折回，八次后继续取下一个字节。
    for (unsigned bit = 0; bit < 8U; ++bit)
    {
      const bool carry = (crc & 1U) != 0U;
      crc = (uint16_t)(crc >> 1);
      if (carry)
      {
        crc ^= 0xA001U;
      }
    }
  }
  return crc;
}

// 在已有负载后追加低、高两个 CRC 字节。空间不足时返回 false，缓冲区保持原样。
static inline bool append_crc(uint8_t *frame, size_t capacity, size_t length)
{
  // 1. 先判断负载是否落在缓冲区内，再计算剩余空间，避免 size_t 下溢。
  if (frame == NULL || length > capacity || capacity - length < 2U)
  {
    return false;
  }

  // 2. 计算值交给发送缓冲区，按 Modbus RTU 帧序先写低字节。
  const uint16_t crc = crc16_modbus(frame, length);
  frame[length] = (uint8_t)crc;
  frame[length + 1U] = (uint8_t)(crc >> 8);
  return true;
}

// length 是含 CRC 的总长度。调用者先确定帧边界，返回值只代表这段数据的 CRC 是否通过。
static inline bool valid_crc(const uint8_t *frame, size_t length)
{
  if (frame == NULL || length < 2U)
  {
    return false;
  }

  // 1. 用负载重算 CRC，再组装帧尾的低、高字节比较。
  const size_t payload_length = length - 2U;
  const uint16_t received =
    (uint16_t)((uint16_t)frame[payload_length] | ((uint16_t)frame[payload_length + 1U] << 8));
  return received == crc16_modbus(frame, payload_length);
}

#endif
