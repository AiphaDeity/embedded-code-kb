#include "acceleration.h"
#include <assert.h>
#include <stdio.h>

int main(void)
{
  const uint8_t normal[] = {0x40, 0x00, 0xC0, 0x00, 0x00, 0x01};
  Acceleration value = accel_decode(normal);
  assert(value.x == 16384 && value.y == -16384 && value.z == 1);

  const uint8_t limits[] = {0x80, 0x00, 0x7F, 0xFF, 0xFF, 0xFF};
  value = accel_decode(limits);
  assert(value.x == INT16_MIN && value.y == INT16_MAX && value.z == -1);
  puts("MPU6050: high-byte first, signed axes, limits and -1");
}
