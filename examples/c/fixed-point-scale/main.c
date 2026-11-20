#include <assert.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

typedef struct
{
  int32_t numerator, denominator;
  int32_t offset_mc, min_mc, max_mc;
} Scale;

// 将有符号原始计数换算到毫摄氏度，四舍五入后加偏移并限幅。
static bool apply_scale(const Scale *scale, int32_t raw, int32_t *out_mc)
{
  // 1. 拒绝空指针、无效比例和颠倒的量程。失败时保持输出原值。
  if (scale == NULL || out_mc == NULL ||
      scale->denominator <= 0 || scale->min_mc > scale->max_mc)
  {
    return false;
  }

  // 2. 先扩到 64 位，再乘和除。半单位遇到正负值都向远离 0 的方向舍入。
  const int64_t product = (int64_t)raw * scale->numerator;
  const int64_t divisor = scale->denominator;
  int64_t rounded = product / divisor;
  const int64_t twice_remainder = (product % divisor) * 2;
  if (twice_remainder >= divisor) { ++rounded; }
  else if (twice_remainder <= -divisor) { --rounded; }

  // 3. 加入以毫摄氏度给出的偏移。按配置限幅后才缩窄并写给调用者。
  int64_t calibrated = rounded + scale->offset_mc;
  if (calibrated < scale->min_mc) { calibrated = scale->min_mc; }
  if (calibrated > scale->max_mc) { calibrated = scale->max_mc; }
  *out_mc = (int32_t)calibrated;
  return true;
}

int main(void)
{
  // 1. 每计数 1/16 °C，换成毫摄氏度的比例为 1000/16。
  Scale scale = {.numerator = 1000, .denominator = 16, .min_mc = -40000, .max_mc = 125000};
  int32_t reading = 0;
  assert(apply_scale(&scale, 1, &reading) && reading == 63);
  assert(apply_scale(&scale, -1, &reading) && reading == -63);
  assert(apply_scale(&scale, 2, &reading) && reading == 125);

  // 2. 加 125 m°C 补偿。正常值、负数和两端超范围值都交给同一换算。
  scale.offset_mc = 125;
  assert(apply_scale(&scale, 16, &reading) && reading == 1125);
  printf("raw=16 -> %" PRId32 " mC\n", reading);
  assert(apply_scale(&scale, INT32_MAX, &reading) && reading == 125000);
  assert(apply_scale(&scale, INT32_MIN, &reading) && reading == -40000);

  // 3. 配置非法时停止换算，保留旧测量。还检查整数乘法的两个极值。
  scale.denominator = 0;
  assert(!apply_scale(&scale, 0, &reading) && reading == -40000);
  scale = (Scale){.numerator = INT32_MIN, .denominator = 1,
                 .min_mc = INT32_MIN, .max_mc = INT32_MAX};
  assert(apply_scale(&scale, INT32_MIN, &reading) && reading == INT32_MAX);
  assert(apply_scale(&scale, INT32_MAX, &reading) && reading == INT32_MIN);
  puts("fixed point: signed rounding, calibration, saturation and invalid scale");
}
