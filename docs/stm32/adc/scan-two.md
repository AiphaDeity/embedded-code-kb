# 06 · ADC 扫描两路，每轮交付一组结果

一次软件触发依次转换 PC0 和 PC1。DMA 收齐两个半字后，主循环发布一对读数。

## 板型和接线

接两个 10 kΩ 电位器，滑动端分别接 PC0、PC1，两端接板的模拟参考电源和地。模拟输入保持在 0～VDDA。

## 沿执行路径读代码

1. ADC1 的规则序列是 Rank1 通道 10 和 Rank2 通道 11。
2. 启动一次普通模式 DMA，缓冲索引 0、1 对应该次序列的两个 Rank。
3. 完整回调后停止本次 DMA，随后发布两值。出错时保留上次数据，停止失败时停机供调试。

## 完整源码

<a href="../../../examples/stm32/adc/scan-two/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/adc/scan-two/main.c

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3，将固件包路径换成本机路径。

```sh
cmake -S examples/stm32/adc/scan-two -B .work/sky-adc-scan-two -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-adc-scan-two
```

用 SWD 下载 `.work/sky-adc-scan-two/sky-adc-scan-two.elf`，复位运行。将 PC0 拉到接地端，PC1 调到约一半参考电压。预计 input_pc0 接近 0、input_pc1 接近 2048。各自调节时再观察对应值。

## 练习

**预测题**　交换两个 Rank 配置，数组下标代表的输入随之如何变化？

<details><summary>核对预测</summary>

DMA 按转换发生顺序写入。Rank1 变成通道 11 时，数组 0 就来自 PC1。

</details>

**改动题**　交换两个 Rank，并同步更新主循环的发布逻辑。

<details><summary>核对改法</summary>

保持 input_pc0 对应 PC0，input_pc1 对应 PC1。检查排序改动后，转动哪一路仍只明显影响自己的观测值。

</details>

## 验证记录

已用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译链接。信号与观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
