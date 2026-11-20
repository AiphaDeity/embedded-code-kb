#ifndef KB_MPU6050_ACCEL_H
#define KB_MPU6050_ACCEL_H

#include <stdint.h>

typedef struct
{
  int16_t x, y, z;
} Acceleration;

// MPU6050 每个轴以高字节在前的 16 位补码报告。先扩展，减去 65536 后再窄化。
static inline int16_t accel_axis(const uint8_t bytes[2])
{
  const int32_t value = ((int32_t)bytes[0] << 8U) | bytes[1];
  return (int16_t)(value >= 32768 ? value - 65536 : value);
}

// 只接收已由总线完整读回的六个字节。输出为原始计数。
static inline Acceleration accel_decode(const uint8_t bytes[6])
{
  return (Acceleration){accel_axis(bytes), accel_axis(bytes + 2), accel_axis(bytes + 4)};
}

#endif
