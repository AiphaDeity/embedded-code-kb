# 06 · TIM2 触发 ADC1，DMA 轮流交付半缓冲

TIM2 每 10 ms 触发一次 ADC1。DMA 向 16 个半字循环写入，CPU 每次只处理已写完的 8 个采样。

## 板型和接线

天空星 STM32F407VET6，10 kΩ 电位器两端接板的模拟参考供电与 GND，滑动端接 PC0。用板上 `3V0` 和 GND 供电时先核对实际板型，ADC 输入保持在 GND 到 VDDA 内。

## 沿执行路径读代码

1. 配置 TIM2 更新事件输出 TRGO，ADC1 选规则通道 10 和 TRGO 上升沿，DMA2 Stream0 Channel0 配置循环搬运。
2. 先启动 ADC＋DMA，再开 TIM2。前半区写满进入 `HAL_ADC_ConvHalfCpltCallback()`，后半区写满进入 `HAL_ADC_ConvCpltCallback()`。
3. 回调从 NDTR 确认 DMA 正在使用另一半，再计算当前半区的平均值。确认后才发布平均值。该处理须在一个半区周期 80 ms 内完成，TIM2 在调试器暂停时冻结。

## 完整源码

<a href="../../../examples/stm32/adc/timer-dma/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/adc/timer-dma/main.c

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3，替换固件包路径后在仓库根目录执行。

```sh
cmake -S examples/stm32/adc/timer-dma -B .work/sky-adc-timer-dma -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-adc-timer-dma
```

通过 SWD 加载 `.work/sky-adc-timer-dma/sky-adc-timer-dma.elf` 后复位运行。转动电位器，调试器查看 `average_raw` 从接近 0 到接近 4095，`completed_halves` 每 80 ms 左右加一。`skipped_halves` 应保持 0，`adc_error` 为 0。

## 练习

**预测题**　DMA 循环写入时，主循环能每次直接对整段数组求平均吗？

<details><summary>核对预测</summary>

DMA 可在 CPU 读取时改写其中一半。这个例子由半满、全满回调在另一半写入期间读取已完成的一半，再交出独立的平均值。

</details>

**改动题**　把采样率改成 50 Hz，算出半区更新间隔。

<details><summary>核对改法</summary>

保持 PSC，令 TIM2 的 `Period` 为 1999。每个采样约 20 ms，八个样本约 160 ms。

</details>

## 验证记录

目标用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。接线与观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
