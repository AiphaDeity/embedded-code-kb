.syntax unified
.thumb
.section .text.enter_app,"ax",%progbits
.global enter_app
.type enter_app, %function
.thumb_func
enter_app:
    /* 1. 从 r0 装 App 栈顶并选择主栈。此后不再访问 Boot 的栈。 */
    msr msp, r0
    movs r2, #0
    msr control, r2
    isb

    /* 2. 具体 IRQ 仍处于禁止状态。恢复全局 IRQ 掩码后跳到 r1 的 Thumb 入口。 */
    cpsie i
    bx r1
.size enter_app, .-enter_app
