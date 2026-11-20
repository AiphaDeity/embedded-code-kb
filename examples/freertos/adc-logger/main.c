#include "demo.h"
#include "queue.h"
#include "../../stm32/common/sky-adc1-pc0.h"
#include "../../stm32/common/sky-usart1.h"
#include <stdbool.h>
#include <stdio.h>

enum { SAMPLE_QUEUE_DEPTH = 4, PERIOD_MS = 100U };

typedef struct
{
  uint32_t sequence, tick;
  uint16_t raw;
} Sample;

static ADC_HandleTypeDef adc;
static UART_HandleTypeDef uart;
static QueueHandle_t sample_queue;
static volatile uint32_t raw_samples, sample_errors, dropped_logs, logged, log_errors;
static volatile uint32_t pause_logger;

static bool convert(uint16_t *raw)
{
  // 1. 一次触发等待一个结果。只有 EOC 到达并且停止成功才交付原始值。
  if (HAL_ADC_Start(&adc) != HAL_OK) { return false; }
  const HAL_StatusTypeDef status = HAL_ADC_PollForConversion(&adc, 20U);
  const uint32_t value = status == HAL_OK ? HAL_ADC_GetValue(&adc) : 0U;
  const HAL_StatusTypeDef stop = HAL_ADC_Stop(&adc);
  if (status != HAL_OK || stop != HAL_OK || value > 4095U) { return false; }
  *raw = (uint16_t)value;
  return true;
}

static void sample_pc0(void *argument)
{
  (void)argument;
  uint32_t sequence = 0U;
  TickType_t boundary = xTaskGetTickCount();
  for (;;)
  {
    // 1. 按固定周期启动转换。失败时只记本轮错误，下一周期继续。
    vTaskDelayUntil(&boundary, pdMS_TO_TICKS(PERIOD_MS));
    Sample sample = {.sequence = ++sequence};
    if (!convert(&sample.raw))
    {
      ++sample_errors;
      continue;
    }

    // 2. 成功后记录完成 tick，把样本按值交给日志任务。队列满则丢掉这次记录，不延迟采样。
    ++raw_samples;
    sample.tick = (uint32_t)xTaskGetTickCount();
    if (xQueueSend(sample_queue, &sample, 0U) != pdPASS) { ++dropped_logs; }
  }
}

static void write_logs(void *argument)
{
  (void)argument;
  for (;;)
  {
    // 1. 空队列时让出 CPU。调试时暂停日志任务的消费，采样任务仍保持周期。
    if (pause_logger != 0U)
    {
      vTaskDelay(pdMS_TO_TICKS(10U));
      continue;
    }
    Sample sample;
    if (xQueueReceive(sample_queue, &sample, pdMS_TO_TICKS(10U)) != pdPASS) { continue; }

    // 2. 日志任务只使用自己的副本。格式化或发送失败就计数，随后消费下一条。
    char line[40];
    const int length = snprintf(line, sizeof(line), "%lu,%lu,%u\r\n",
                                (unsigned long)sample.sequence, (unsigned long)sample.tick,
                                (unsigned)sample.raw);
    if (length > 0 && (size_t)length < sizeof(line) &&
        HAL_UART_Transmit(&uart, (const uint8_t *)line, (uint16_t)length, 20U) == HAL_OK)
    {
      ++logged;
    }
    else
    {
      ++log_errors;
    }
  }
}

int main(void)
{
  // 1. 先准备 ADC、串口与队列。任一步初始化失败，调度器不启动。
  demo_init();
  configASSERT(sky_adc1_pc0_init(&adc) == HAL_OK);
  configASSERT(sky_usart1_init(&uart) == HAL_OK);
  sample_queue = xQueueCreate(SAMPLE_QUEUE_DEPTH, sizeof(Sample));
  configASSERT(sample_queue != NULL);

  // 2. 采样任务优先，串口日志只在独立任务里发。调度器返回时进入故障循环。
  configASSERT(xTaskCreate(sample_pc0, "sample", 256U, NULL, 2U, NULL) == pdPASS);
  configASSERT(xTaskCreate(write_logs, "logger", 512U, NULL, 1U, NULL) == pdPASS);
  vTaskStartScheduler();
  demo_fault();
}
