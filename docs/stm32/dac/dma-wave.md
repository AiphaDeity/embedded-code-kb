# 11 · TIM6 和 DMA 让 DAC 持续输出查表波形

16 个采样点放在普通 SRAM。TIM6 以标称 16 kHz 触发 DAC，PA4 输出约 1 kHz 的阶梯正弦。

## 板型和接线

PA4 接示波器的高阻输入，示波器地接板地。实际电压幅度取决于参考电压和负载。

## 沿执行路径读代码

1. DMA1 Stream5 Channel7 按半字从 SRAM 循环传给 DAC。
2. 先准备 DMA 并预置输出，再启动 TIM6 TRGO。
3. DMA 错误或 DAC 欠载时，IRQ 停止触发，主循环停止 DMA 并留下错误码。

## 完整源码

<a href="../../../examples/stm32/dac/dma-wave/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/dac/dma-wave/main.c

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。在仓库根目录执行以下命令，替换固件包路径。

```sh
cmake -S examples/stm32/dac/dma-wave -B .work/sky-dac-dma-wave -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-dac-dma-wave
```

通过 SWD 加载 `.work/sky-dac-dma-wave/sky-dac-dma-wave.elf` 后复位。预计约 1 ms 一轮，中心值约一半参考电压。正常运行时 stage 和 error 都为 0。

## 练习

**预测题**　16 kHz 触发为什么只生成约 1 kHz 的波形？

<details><summary>核对预测</summary>

16 次触发遍历一份查表，16000 除以 16 得到约 1000 Hz。

</details>

**改动题**　把 TIM6 Period 从 999 改为 1999。

<details><summary>核对改法</summary>

样本率约变成 8 kHz，波形约 500 Hz。

</details>

## 验证记录

已用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。接线及输出待实板核对。 [ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
