# 04 · NVIC / EXTI 按键边沿计数

按住按键时，LED 只在确认一次上升沿后切换。主循环和触发中断的代码各自做一件事，可以同时打开 Watch 观察边沿与已处理事件。

处理两条线共用一个中断入口时，继续读[共享 EXTI 的分派与优先级](shared-lines.md)。

## 板型和接线

天空星核心板自带 PA0 用户键（按下高电平）和 PB2 LED（高电平亮）。接 USB 供电和 SWD 即可。

## 执行顺序

1. 先配置 LED，再把 PA0 配为 EXTI0 上升沿输入，最后开放 NVIC。
2. 按下时 `EXTI0_IRQHandler()` 调用 HAL 清除 pending 标志，随后执行 GPIO 回调。
3. 回调只增加 `edges`。主循环发现计数变化后检查距上一次边沿的时间，满足 30 ms 才更新 LED。

## 完整源码

<a href="../../../examples/stm32/exti/button-irq/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/exti/button-irq/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包路径替换为本机路径，在仓库根目录执行：

```sh
cmake -S examples/stm32/exti/button-irq -B .work/sky-exti-button-irq -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-exti-button-irq
```

用实验板通过 SWD 加载 `.work/sky-exti-button-irq/sky-exti-button-irq.elf` 并复位运行。每次松开后间隔至少 100 ms 再按，观察 LED 每按一次切换一次。`edges` 可能因按键抖动增加多次，`accepted` 记录防抖窗口外的事件。

## 练习

**预测题**　为什么在回调里只计数？

<details><summary>核对预测</summary>

退出中断前保留事件即可。耗时判断与 LED 操作放在主循环，避免拉长 ISR。

</details>

**改动题**　把防抖间隔改为 100 ms，比较快速连按时的两个计数。

<details><summary>核对改法</summary>

短于窗口的边沿仍记录到 `edges`，但不再增加 `accepted`。这是一种按边沿间隔过滤的示例。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译及链接。接线、时序和观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
