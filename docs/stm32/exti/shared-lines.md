# 04 · 在 EXTI9_5 共享中断中区分 EXTI6 与 EXTI7

PB6 和 PB7 共用一个 NVIC 入口，PA0 有独立入口。固件同时制造三个挂起位，再分别查看首先响应的入口和三条线路的计数。

## 板型和接线

使用天空星 STM32F407VET6。PA0 使用板载按键。PB6、PB7 各接一个常开按钮到 GND，由内部上拉提供默认高电平。首次运行先松开所有按键，让三条计数的初始变化只来自软件触发。

## 沿执行路径读代码

1. `init_inputs()` 把 PA0 映射到 EXTI0 上升沿，把 PB6、PB7 映射到 EXTI6、EXTI7 下降沿。后两条共享 `EXTI9_5_IRQn`。
2. 主函数在屏蔽中断期间通过 `SWIER` 一次触发三条线。使能中断后，优先级数值更小的 EXTI0 先进入。`first_irq` 记录最先进入的入口。
3. `EXTI9_5_IRQHandler()` 分别检查第 6、7 线。HAL 针对挂起的线先清除一位，再调用 `HAL_GPIO_EXTI_Callback()`。两线同时挂起时两条计数各增加一次。
4. 主循环在短临界区取三路计数，再写到 `observed` 给调试器读取。按键抖动可能产生多个边沿，每条 pending 只占一位。需要稳定按键事件时可继续读[按键消抖](../gpio/debounced-button.md)。

各 IRQ 的映射与软件挂起见 [ST RM0090 EXTI 章节](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)。

## 完整源码

<a href="../../../examples/stm32/exti/shared-lines/main.c" download="main.c">下载 main.c</a> · <a href="../../../examples/stm32/exti/shared-lines/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../../examples/stm32/exti/shared-lines/main.c

## 构建与观察

```sh
cmake -S examples/stm32/exti/shared-lines -B .work/sky-exti-shared-lines -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-exti-shared-lines
```

通过 SWD 加载 `.work/sky-exti-shared-lines/sky-exti-shared-lines.elf`，从复位运行。

三条线的软件触发处理完成后，预期 `first_irq=1`，`observed` 的三个计数都为 1。单独按 PB6 或 PB7 时只观察对应计数的增加。

## 练习

**预测题**　两条共享线同时挂起，一个 `if / else if` 每次进入只清一条，另一条会发生什么？

<details><summary>核对预测</summary>

另一条 pending 仍在。NVIC 可能再次进入共享中断处理它。对每条线分别检查和清除，可在同一次处理函数中消费两个挂起位。

</details>

**改动题**　将 EXTI9_5 优先级改为 4，再核对三项计数和 `first_irq`。

<details><summary>核对改法</summary>

从复位运行，三个挂起位预期仍各计一次。共享入口现在优先级更高，`first_irq` 应为 2。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译链接。天空星上的 SWIER 自测与外部按键待实板。

在 QEMU Cortex-M4 中注入三条输入边沿后，读到挂起掩码 `0xC1`，三个计数各为 1。分别使用两组优先级，`first_irq` 为 1 和 2。该版本模拟器中的 `SWIER` 写入只更新寄存器值，天空星上的软件触发仍待实板核对。
