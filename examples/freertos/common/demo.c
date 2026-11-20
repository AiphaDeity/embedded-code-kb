#include "demo.h"

void demo_init(void)
{
  HAL_Init();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_2, GPIO_PIN_RESET);
  GPIO_InitTypeDef pin = {.Pin = GPIO_PIN_2, .Mode = GPIO_MODE_OUTPUT_PP};
  HAL_GPIO_Init(GPIOB, &pin);
}

void demo_fault(void)
{
  __disable_irq();
  for (;;) {}
}

void xPortSysTickHandler(void);
void SysTick_Handler(void)
{
  // 1. HAL 初始化阶段先用自己的节拍。调度器启动后，同一个中断再交给内核。
  HAL_IncTick();
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
    xPortSysTickHandler();
  }
}
