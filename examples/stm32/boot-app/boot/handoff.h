#ifndef SKY_HANDOFF_H
#define SKY_HANDOFF_H

#include "stm32f4xx.h"
#include "image.h"

// ARM 调用约定在 r0/r1 中传递栈顶与入口。汇编接手后不返回 C。
_Noreturn void enter_app(uint32_t main_stack, uint32_t reset_vector);

static inline _Noreturn void handoff_to(uint32_t vector, AppEntry entry)
{
  // 1. 进入交接阶段。关闭 SysTick、外部 IRQ 与挂起标志。此 Boot 没有启动 DMA 或其他外设请求。
  __disable_irq();
  SysTick->CTRL = 0U;
  SysTick->LOAD = 0U;
  SysTick->VAL = 0U;
  const uint32_t nvic_banks = (SCnSCB->ICTR & SCnSCB_ICTR_INTLINESNUM_Msk) + 1U;
  for (uint32_t bank = 0; bank < nvic_banks; ++bank)
  {
    NVIC->ICER[bank] = UINT32_MAX;
    NVIC->ICPR[bank] = UINT32_MAX;
  }
  SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;

  // 2. CPU 改用 App 向量表。汇编接手装入 MSP，再跳到 App 的复位入口。
  SCB->VTOR = vector;
  __set_BASEPRI(0U);
  __DSB();
  __ISB();
  enter_app(entry.main_stack, entry.reset_vector);
}
#endif
