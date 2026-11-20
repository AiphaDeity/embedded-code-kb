#include "stm32f4xx.h"
#include "image.h"
#include "handoff.h"

static volatile ImageResult boot_result;

static void show_failure(void)
{
  RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
  (void)RCC->AHB1ENR;
  GPIOB->BSRR = 1U << 2;
  GPIOB->MODER = (GPIOB->MODER & ~(3U << 4)) | (1U << 4);
}

int main(void)
{
  // 1. 初始时钟保持复位状态。读取 App 描述符及分区，校验失败就在 Boot 点亮 LED。
  AppEntry entry;
  boot_result = validate_image((const uint8_t *)APP_HEADER_ADDR, (const uint8_t *)APP_ADDR,
                               APP_CAPACITY, &entry);
  if (boot_result != IMAGE_OK)
  {
    show_failure();
    for (;;) { __WFI(); }
  }

  // 2. 清理 Boot 的中断后，切换向量表和栈，跳转到已校验的复位入口。
  handoff_to(APP_ADDR, entry);
}
