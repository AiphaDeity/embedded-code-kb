#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

enum { WINDOW_SIZE = 4, ADC_MAX = 4095, WARN_ON = 3000, WARN_OFF = 2700 };

typedef struct
{
  uint16_t samples[WINDOW_SIZE];
  uint32_t sum;
  unsigned next;
  unsigned count;
  bool warning;
} Filter;

typedef struct
{
  uint16_t average;
  bool warning;
} Reading;

// 从零初始化的 Filter 开始。输入单位为 ADC 计数，超范围时状态与输出都保持原值。
static bool update(Filter *filter, uint16_t sample, Reading *result)
{
  // 1. 先拒绝无效采样，调用者可继续显示前一次结果。
  if (sample > ADC_MAX)
  {
    return false;
  }

  // 2. 环形位置指向最旧的样本。窗口填满后，先从总和中减去将被覆盖的值。
  if (filter->count == WINDOW_SIZE)
  {
    filter->sum -= filter->samples[filter->next];
  }
  else
  {
    ++filter->count;
  }
  filter->samples[filter->next] = sample;
  filter->sum += sample;
  filter->next = (filter->next + 1U) % WINDOW_SIZE;

  // 3. 启动时用实际样本数平均。跨过对应阈值才改变告警，中间区间沿用原状态。
  const uint16_t average = (uint16_t)(filter->sum / filter->count);
  if (average >= WARN_ON)
  {
    filter->warning = true;
  }
  else if (average <= WARN_OFF)
  {
    filter->warning = false;
  }
  *result = (Reading){.average = average, .warning = filter->warning};
  return true;
}

int main(void)
{
  // 1. 逐次模拟 ADC 输入，让窗口先拉高告警，再经过两个阈值间的区域。
  Filter filter = {0};
  Reading result = {0};
  assert(update(&filter, 3200U, &result) && result.average == 3200U && result.warning);
  assert(update(&filter, 2600U, &result) && result.average == 2900U && result.warning);
  assert(update(&filter, 2600U, &result) && result.average == 2800U && result.warning);
  assert(update(&filter, 2600U, &result) && result.average == 2750U && result.warning);

  // 2. 第五次覆盖旧的 3200，低于解除阈值。无效样本随后被丢掉。
  assert(update(&filter, 2600U, &result) && result.average == 2600U && !result.warning);
  assert(!update(&filter, 5000U, &result));
  assert(result.average == 2600U && !result.warning && filter.next == 1U);
  assert(update(&filter, 4095U, &result) && result.average == 2973U && !result.warning);
  puts("sample filter: warm-up, wrap, hysteresis, invalid sample");
  return 0;
}
