# 09 · TIM3 读取两相编码器并累计位置

TIM3 从 A/B 相位差判断方向。主循环定期读取 16 位计数，把回绕差累加到位置。

## 板型和接线

3.3 V 兼容编码器 A 接 PA6、B 接 PA7，并共地。机械触点可接地配合例子中的上拉。

## 沿执行路径读代码

1. 配置 TIM3 AF2、双路编码器模式和数字滤波。
2. TIM3 按两路边沿增减，main 每约 10 ms 读 CNT。
3. 把 16 位模差映射为有符号位移。每个采样间隔内，净移动绝对值需小于 32768。

## 完整源码

<a href="../../../examples/stm32/tim/encoder/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/tim/encoder/main.c

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。在仓库根目录执行以下命令，替换固件包路径。

```sh
cmake -S examples/stm32/tim/encoder -B .work/sky-tim-encoder -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-tim-encoder
```

通过 SWD 加载 `.work/sky-tim-encoder/sky-tim-encoder.elf` 后复位。转动时看 position 的正负方向变化。可临时将起始 CNT 设到 65534，确认跨回绕继续累计。

## 练习

**预测题**　上一值 65534，本次 2，净前进几个计数？

<details><summary>核对预测</summary>

16 位模差为 4，属于正向区间。

</details>

**改动题**　交换 A/B 两根线，再向原方向转。

<details><summary>核对改法</summary>

相位顺序翻转，净移动符号也翻转。

</details>

## 验证记录

已用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。接线及输出待实板核对。 [ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
