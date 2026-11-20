#include "stm32f4xx_hal.h"
#include "../../common/clock84.h"
#include "logger.h"
#include "sdio_disk.h"

static FATFS file_system;
static volatile FRESULT log_status;
static volatile unsigned stage;

void SysTick_Handler(void) { HAL_IncTick(); }

int main(void)
{
  // 1. PLLQ 为 SDIO 提供标称 48 MHz。配置失败时保留阶段号。
  HAL_Init();
  stage = 1U;
  if (sky_clock84() != HAL_OK) { for (;;) {} }
  __HAL_RCC_GPIOB_CLK_ENABLE();
  GPIO_InitTypeDef led = {.Pin = GPIO_PIN_2, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  HAL_GPIO_Init(GPIOB, &led);

  // 2. 每次复位只追加一行。FatFs 通过 diskio 调用 HAL，文件关闭后才报告成功。
  stage = 2U;
  static const char line[] = "sky: sd log\r\n";
  log_status = append_log(&file_system, line, sizeof(line) - 1U);
  if (log_status == FR_OK)
  {
    stage = 3U;
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_SET);
  }
  for (;;)
  {
    // 3. 停在可观察状态。失败时调试器读 log_status 与 sd_error。
    __WFI();
  }
}
