#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef void (*HealthChanged)(void *context, bool unhealthy);

typedef struct
{
  unsigned failures_to_alarm, successes_to_clear;
  HealthChanged on_change;
  void *context;
} HealthConfig;

typedef struct
{
  HealthConfig config;
  unsigned streak;
  bool unhealthy;
} HealthMonitor;

// 阈值以连续轮数计且必须大于 0。失败时不改 monitor。context 在通知期间必须有效。
static bool health_init(HealthMonitor *monitor, const HealthConfig *config)
{
  if (monitor == NULL || config == NULL ||
      config->failures_to_alarm == 0U || config->successes_to_clear == 0U)
  {
    return false;
  }
  *monitor = (HealthMonitor){.config = *config};
  return true;
}

// 接收一次操作的成功或失败。由同一任务串行调用，回调不重入本实例。
static void health_record(HealthMonitor *monitor, bool ok)
{
  // 1. 当前健康时累计失败，已告警时累计成功。输入与当前状态一致时清零。
  const bool unhealthy_now = !ok;
  if (unhealthy_now == monitor->unhealthy)
  {
    monitor->streak = 0U;
    return;
  }
  const unsigned threshold = monitor->unhealthy ?
    monitor->config.successes_to_clear : monitor->config.failures_to_alarm;
  if (++monitor->streak < threshold) { return; }

  // 2. 先切换状态、清计数，再同步调用可选回调。后续相同输入不会重复报告。
  monitor->unhealthy = unhealthy_now;
  monitor->streak = 0U;
  if (monitor->config.on_change != NULL)
  {
    monitor->config.on_change(monitor->config.context, monitor->unhealthy);
  }
}

typedef struct
{
  unsigned calls;
  bool last_unhealthy;
} EventLog;

static void record_change(void *context, bool unhealthy)
{
  EventLog *log = context;
  log->last_unhealthy = unhealthy;
  ++log->calls;
  puts(unhealthy ? "alarm" : "recovered");
}

int main(void)
{
  // 1. 两个实例接同一回调，各自传入自己的日志。修改局部配置不影响已初始化的实例。
  EventLog first = {0}, second = {0};
  HealthConfig config = {.failures_to_alarm = 2U, .successes_to_clear = 3U,
                        .on_change = record_change, .context = &first};
  HealthMonitor sensor, link;
  if (!health_init(&sensor, &config)) { return 1; }
  config.failures_to_alarm = 1U;
  config.context = &second;
  if (!health_init(&link, &config)) { return 1; }

  // 2. 连续两次失败只报告一次。另一实例独立计数，读取不到 sensor 的状态。
  health_record(&sensor, false);
  assert(first.calls == 0U && second.calls == 0U);
  health_record(&link, false);
  assert(first.calls == 0U && second.calls == 1U && second.last_unhealthy);
  health_record(&sensor, false);
  health_record(&sensor, false);
  assert(first.calls == 1U && first.last_unhealthy);

  // 3. 错误配置不能覆盖已有实例。中途一次失败打断“两次成功”的恢复计数。
  config.successes_to_clear = 0U;
  if (health_init(&sensor, &config)) { return 1; }
  assert(sensor.unhealthy && sensor.config.context == &first);
  health_record(&sensor, true);
  health_record(&sensor, true);
  health_record(&sensor, false);
  health_record(&sensor, true);
  health_record(&sensor, true);
  assert(first.calls == 1U);
  health_record(&sensor, true);
  assert(first.calls == 2U && !first.last_unhealthy && second.calls == 1U);

  // 4. 暂时没有事件接收者时，可传空回调并直接读取状态。
  config.successes_to_clear = 1U;
  config.on_change = NULL;
  config.context = NULL;
  HealthMonitor silent;
  if (!health_init(&silent, &config)) { return 1; }
  health_record(&silent, false);
  assert(silent.unhealthy);
  health_record(&silent, true);
  assert(!silent.unhealthy);
  puts("health callbacks: copied config, isolated context, alarm and recovery");
  return 0;
}
