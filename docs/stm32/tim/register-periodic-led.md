# 09 · 用 TIM2 寄存器定时翻转 LED

配置 TIM2 每 500 ms 触发一次 IRQ，再在中断内翻转 PB2。可对照 HAL 时基版的初始化、清标志和中断入口。[HAL TIM2 版](periodic-led.md)

## 板型和接线

使用天空星板载 PB2 LED，连接供电和 SWD。复位使用 HSI，APB1 分频为 /1。

## 沿执行路径读代码

1. 把 `PSC` 设为 15999，`ARR` 设为 499，定时器获得标称 1 kHz 计数。
2. 用 `EGR.UG` 装入分频值并等待 `SR.UIF` 出现。清掉这次软件更新，再打开 NVIC、`DIER.UIE` 和 `CR1.CEN`。
3. TIM2 计满后进入 `TIM2_IRQHandler()`，清 `UIF` 并翻转 LED。主循环回到 `__WFI()` 等下一次中断。

## 完整源码

<a href="../../../examples/stm32/tim/register-periodic-led/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/tim/register-periodic-led/main.c

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3，替换固件包路径后在仓库根目录执行。

```sh
cmake -S examples/stm32/tim/register-periodic-led -B .work/sky-tim-register-led -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-tim-register-led
```

通过 SWD 加载 `.work/sky-tim-register-led/sky-tim-register-led.elf` 后复位运行。观察 LED 约每 0.5 秒翻转，`updates` 每秒约加 2。示波器量 PB2 可看完整周期约 1 秒。

## 练习

**预测题**　为什么初始化时已经出现 `UIF`？

<details><summary>核对预测</summary>

本例用软件更新把预分频值装入定时器，更新事件也会置 `UIF`。清掉它后才启动更新中断。

</details>

**改动题**　把中断间隔改成 250 ms。

<details><summary>核对改法</summary>

保持分频不变，令 `ARR = 249`。全亮灭周期变为约 500 ms。

</details>

## 验证记录

目标用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。接线与观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
