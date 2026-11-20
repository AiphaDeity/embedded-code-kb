# 09 · 输出 1 kHz PWM 并用输入捕获测频

TIM3_CH1 从 PA6 输出约 1 kHz、占空比 25% 的 PWM。TIM2_CH2 从 PA1 捕获两个相邻上升沿，计算得到约 1000 Hz。

## 板型和接线

天空星 STM32F407VET6，取 PA6 接 PA1，可串 1 kΩ。确认两脚接到对应排针且没有其他外设驱动。示波器探头接 PA6，地接板 GND。复位使用 HSI，APB1 分频 /1。

## 沿执行路径读代码

1. TIM2 每微秒加一。先开捕获，PA1 上升沿到来时自动保存 `CNT` 到 `CCR2`。
2. 启动 TIM3，`ARR=999` 决定周期，`CCR1=250` 决定高电平宽度。
3. 捕获回调将两个值相减得到周期。若发生覆盖则放弃旧起点，以后重新收集两个边沿。

## 完整源码

<a href="../../../examples/stm32/tim/pwm-capture/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/tim/pwm-capture/main.c

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3，替换固件包路径后在仓库根目录执行。

```sh
cmake -S examples/stm32/tim/pwm-capture -B .work/sky-tim-pwm-capture -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-tim-pwm-capture
```

通过 SWD 加载 `.work/sky-tim-pwm-capture/sky-tim-pwm-capture.elf` 后复位运行。示波器预计量到周期约 1 ms、高电平约 250 μs。调试器中 `period_ticks` 约 1000，`measured_hz` 约 1000，`valid_periods` 持续增加。断线后计数停止增长，最后一次频率保留。

## 练习

**预测题**　第一个上升沿就能算出频率吗？

<details><summary>核对预测</summary>

第一个边沿只建立时间起点。至少两次有效捕获才得到一段周期。

</details>

**改动题**　把占空比改成 60%，预测测得频率。

<details><summary>核对改法</summary>

只把 `Pulse` 改成 600，示波器可观察高电平约 600 μs。两个相邻上升沿的间隔仍约 1000 μs。

</details>

## 验证记录

目标用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。接线与观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) · [STM32F407 数据手册](https://www.st.com/resource/en/datasheet/stm32f407ve.pdf)
