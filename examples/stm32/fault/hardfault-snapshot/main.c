#include "stm32f407xx.h"
#include <stdbool.h>
#include <stdint.h>

extern uint32_t _estack;

typedef struct
{
  uint32_t r0, r1, r2, r3, r12, lr, pc, xpsr;
} BasicFrame;

typedef struct
{
  uint32_t exc_return, frame_address;
  uint32_t hfsr, cfsr, mmfar, bfar;
  uint32_t has_basic_frame, complete;
  BasicFrame frame;
} FaultReport;

static volatile FaultReport fault_report;

__attribute__((noinline, noreturn))
static void fault_halt(void)
{
  for (;;) { __NOP(); }
}

// used 保留供裸汇编跳转的入口。此例只读取普通 SRAM 中的基本异常帧。
__attribute__((used, noinline, noreturn))
static void fault_capture(const BasicFrame *frame, uint32_t exc_return)
{
  // 1. 先保存异常返回值、现场地址和故障位。只有有效位已置位才读对应地址。
  const uint32_t cfsr = SCB->CFSR;
  fault_report.exc_return = exc_return;
  fault_report.frame_address = (uint32_t)(uintptr_t)frame;
  fault_report.hfsr = SCB->HFSR;
  fault_report.cfsr = cfsr;
  if ((cfsr & SCB_CFSR_MMARVALID_Msk) != 0U) { fault_report.mmfar = SCB->MMFAR; }
  if ((cfsr & SCB_CFSR_BFARVALID_Msk) != 0U) { fault_report.bfar = SCB->BFAR; }

  // 2. 确认硬件压栈成功、帧格式和地址匹配，再读取寄存器现场。
  const uintptr_t address = (uintptr_t)frame;
  const uintptr_t sram_end = (uintptr_t)&_estack;
  const uint32_t stack_error = SCB_CFSR_MSTKERR_Msk | SCB_CFSR_STKERR_Msk |
                               SCB_CFSR_MUNSTKERR_Msk | SCB_CFSR_UNSTKERR_Msk;
  const bool basic = (exc_return & (1UL << 4)) != 0U;
  if (basic && (cfsr & stack_error) == 0U &&
      address >= SRAM_BASE && address <= sram_end - sizeof(*frame) &&
      (address & 3U) == 0U)
  {
    fault_report.frame = *frame;
    fault_report.has_basic_frame = 1U;
  }

  // 3. 标记快照完成后停在 fault_halt。调试器从 fault_report 中读取数据。
  fault_report.complete = 1U;
  fault_halt();
}

__attribute__((naked))
void HardFault_Handler(void)
{
  // 1. 异常入口的 LR 是 EXC_RETURN。取 CPU 实际入栈的 SP，放入 C 参数寄存器。
  __asm volatile(
    "tst lr, #4\n"
    "ite eq\n"
    "mrseq r0, msp\n"
    "mrsne r0, psp\n"
    "mov r1, lr\n"
    "b fault_capture\n");
}

__attribute__((noinline, noreturn))
static void trigger_fault(void)
{
  __asm volatile("udf #0");
  for (;;) { __NOP(); }
}

int main(void)
{
  // 1. 从复位运行时关闭独立 UsageFault。故意执行未定义指令以观察升级后的 HardFault。
  SCB->SHCSR &= ~SCB_SHCSR_USGFAULTENA_Msk;
  __DSB();
  __ISB();
  trigger_fault();
}
