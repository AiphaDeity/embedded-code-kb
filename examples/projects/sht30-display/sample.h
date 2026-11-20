#ifndef KB_SHT30_SAMPLE_H
#define KB_SHT30_SAMPLE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
  int16_t temperature_tenths;
  uint16_t humidity_tenths;
} Sht30Sample;

static inline uint8_t sht30_crc(const uint8_t word[2])
{
  uint8_t crc = 0xFFU;
  for (unsigned i = 0; i < 2U; ++i)
  {
    crc ^= word[i];
    for (unsigned bit = 0; bit < 8U; ++bit)
    {
      const uint8_t polynomial = (crc & 0x80U) != 0U ? 0x31U : 0U;
      crc = (uint8_t)((crc << 1U) ^ polynomial);
    }
  }
  return crc;
}

// 两组 CRC 都通过才更新输出。单位分别为 0.1 °C 和 0.1 %RH。
static inline bool sht30_decode(const uint8_t frame[6], Sht30Sample *out)
{
  if (sht30_crc(frame) != frame[2] || sht30_crc(frame + 3) != frame[5])
  {
    return false;
  }
  const uint32_t temperature = ((uint32_t)frame[0] << 8U) | frame[1];
  const uint32_t humidity = ((uint32_t)frame[3] << 8U) | frame[4];
  out->temperature_tenths = (int16_t)((int)((temperature * 1750U + 32767U) / 65535U) - 450);
  out->humidity_tenths = (uint16_t)((humidity * 1000U + 32767U) / 65535U);
  return true;
}

// [0, maximum] 映射到 [0, width] 个像素。负值和超范围值在两端截断。
static inline uint16_t bar_width(int value, uint16_t maximum, uint16_t width)
{
  if (value <= 0) { return 0; }
  if ((unsigned)value >= maximum) { return width; }
  return (uint16_t)(((uint32_t)value * width) / maximum);
}

#endif
