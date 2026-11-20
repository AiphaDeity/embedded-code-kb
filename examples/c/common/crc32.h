#ifndef KB_CRC32_H
#define KB_CRC32_H

#include <stddef.h>
#include <stdint.h>

// CRC-32/ISO-HDLC。size > 0 时 data 至少指向 size 个可读字节。
static inline uint32_t kb_crc32(const uint8_t *data, size_t size)
{
  uint32_t crc = UINT32_MAX;
  for (size_t i = 0; i < size; ++i)
  {
    crc ^= data[i];
    for (unsigned bit = 0; bit < 8U; ++bit)
    {
      const uint32_t polynomial = (crc & 1U) ? 0xEDB88320U : 0U;
      crc = (crc >> 1) ^ polynomial;
    }
  }
  return crc ^ UINT32_MAX;
}
#endif
