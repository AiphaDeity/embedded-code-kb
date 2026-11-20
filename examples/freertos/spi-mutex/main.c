#include "demo.h"
#include "semphr.h"
#include "../../stm32/common/sky-spi1-mode0.h"
#include <stdbool.h>

typedef struct
{
  uint32_t id;
  GPIO_TypeDef *port;
  uint16_t pin;
} Device;

typedef struct
{
  uint32_t attempted, ids_ok, other_ids, transfer_errors, lock_timeouts;
  uint8_t last_id[3];
} DeviceStats;

static const Device devices[] = {{0U, GPIOA, GPIO_PIN_4}, {1U, GPIOB, GPIO_PIN_0}};
static SPI_HandleTypeDef spi;
static SemaphoreHandle_t bus_mutex;
static volatile DeviceStats stats[2];
static volatile uint32_t hold_device_a, abort_errors;
static volatile bool bus_stopped;

static void init_select(const Device *device)
{
  // 1. 先给输出锁存器写高，切到输出时片选保持不激活。
  HAL_GPIO_WritePin(device->port, device->pin, GPIO_PIN_SET);
  GPIO_InitTypeDef pin = {.Pin = device->pin, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(device->port, &pin);
}

// 调用者持锁。释放片选之后才发布结果或中止失败事务。
static void read_jedec(const Device *device, volatile DeviceStats *result)
{
  // 1. 拉低一个片选，整段命令和读取都受同一把锁保护。
  const uint8_t command[] = {0x9FU, 0U, 0U, 0U};
  uint8_t reply[sizeof(command)];
  ++result->attempted;
  HAL_GPIO_WritePin(device->port, device->pin, GPIO_PIN_RESET);
  const HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&spi, command, reply, sizeof(reply), 20U);
  HAL_GPIO_WritePin(device->port, device->pin, GPIO_PIN_SET);

  // 2. 失败时在锁内收尾。若中止失败，后续任务只保留现场，不再激活片选。
  if (status != HAL_OK)
  {
    ++result->transfer_errors;
    if (HAL_SPI_Abort(&spi) != HAL_OK)
    {
      ++abort_errors;
      bus_stopped = true;
    }
    return;
  }

  // 3. RX 的第一字节是发命令时收到的内容。有意义的标识从下一字节开始。
  for (unsigned i = 0U; i < 3U; ++i) { result->last_id[i] = reply[i + 1U]; }
  if (reply[1] == 0xEFU && reply[2] == 0x40U && reply[3] == 0x18U)
  {
    ++result->ids_ok;
  }
  else
  {
    ++result->other_ids;
  }
}

static void device_task(void *argument)
{
  const Device *device = argument;
  volatile DeviceStats *result = &stats[device->id];
  for (;;)
  {
    // 1. 最多等待 15 ms。其他任务正在占用时，超时分支完全不碰 SPI 和片选。
    if (xSemaphoreTake(bus_mutex, pdMS_TO_TICKS(15U)) == pdTRUE)
    {
      // 2. 调试注入在片选拉低之前暂停 A，给 B 制造真实的锁等待。
      if (device->id == 0U && hold_device_a != 0U) { vTaskDelay(pdMS_TO_TICKS(80U)); }
      if (!bus_stopped) { read_jedec(device, result); }

      // 3. 不论传输结果均归还锁。下一轮等待在锁外。
      configASSERT(xSemaphoreGive(bus_mutex) == pdTRUE);
    }
    else
    {
      ++result->lock_timeouts;
    }
    vTaskDelay(pdMS_TO_TICKS(40U));
  }
}

int main(void)
{
  // 1. 初始化两条独立片选，再准备唯一的 SPI1 句柄与互斥锁。
  demo_init();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  init_select(&devices[0]);
  init_select(&devices[1]);
  configASSERT(sky_spi1_mode0_init(&spi) == HAL_OK);
  bus_mutex = xSemaphoreCreateMutex();
  configASSERT(bus_mutex != NULL);

  // 2. 任务共用总线，各自持有一组片选与诊断数据。创建完成才进入调度。
  configASSERT(xTaskCreate(device_task, "flash-a", 256U, (void *)&devices[0], 1U, NULL) == pdPASS);
  configASSERT(xTaskCreate(device_task, "flash-b", 256U, (void *)&devices[1], 2U, NULL) == pdPASS);
  vTaskStartScheduler();
  demo_fault();
}
