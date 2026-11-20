# 天空星用 SHT30 采集温湿度并刷新 LCD

每轮读取 SHT30 的温度和湿度，将结果画在 ILI9341 上。上行是温度，下行是湿度，底部色块表示本轮数据是否有效。

## 准备模块和接线

- 天空星 STM32F407VET6 和 3.3 V 兼容的 SHT30 模块。PB6 接 SCL，PB7 接 SDA，ADDR 接低选择 `0x44`，总线加到 3.3 V 的上拉，模块与板共地。
- 16 位 8080 接口 ILI9341 按[并口屏示例](../stm32/fsmc/ili9341-pixel.md)接线。并口使用 PD、PE 和 PC6，可与这里的 I²C1 同时使用。

## 沿执行路径读代码

1. `main()` 初始化 LCD 和 I²C1，保存两份状态。初始化失败时停在主循环前。
2. `measure()` 发送 `24 00`，等待 20 ms，再收六字节。温度的两个字节和 CRC 在前，湿度和 CRC 在后。
3. `sht30_decode()` 先检查两组 CRC，再换算为 0.1 °C 和 0.1 %RH。主循环用一组新值更新两条图，并把状态块画成绿色。
4. 发送、接收或 CRC 失败时，条形图保留上次成功值，状态块变红。等待 500 ms 后继续采样。

## 完整源码

<a href="../../examples/projects/sht30-display/main.c" download="main.c">下载 main.c</a> · <a href="../../examples/projects/sht30-display/sample.h" download="sample.h">下载 CRC 与换算</a> · <a href="../../examples/projects/sht30-display/check.c" download="check.c">下载主机检查</a> · <a href="../../examples/projects/sht30-display/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../examples/projects/sht30-display/main.c

<<< ../../examples/projects/sht30-display/sample.h{c}

## 编译与观察

```sh
cmake -S examples/projects/sht30-display -B .work/sky-sht30-display -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-sht30-display
```

通过 SWD 加载 `.work/sky-sht30-display/sky-sht30-display.elf`。橙色条将 0～50 °C 映射到 0～200 像素，青色条将 0～100 %RH 映射到同样宽度。超出范围会截断，完整测量值在 `last_sample`。看 `sample_status` 可分辨发送、接收和 CRC 错误。

## 练习

**预测题**　湿度 CRC 错误，温度 CRC 正确。这一轮屏幕会怎样？

<details><summary>核对预测</summary>

两条图都保留上次成功值，状态块变红，`samples_failed` 加一。等两组 CRC 都通过再一起更新。

</details>

**改动题**　将温度的满量程改为 40 °C，预测 25 °C 时的宽度。

<details><summary>核对改法</summary>

将主循环传给 `bar_width()` 的最大值改为 `400U`。输入值 250 经 `250 × 200 / 400` 得 125 像素。可把该输入补入 `check.c`。

</details>

## 验证记录

已用 STM32CubeF4 V1.28.3 交叉编译链接。主机核对手册的 `BE EF → 92` CRC、坏帧、负温度、湿度满量程及条形图边界。屏幕、模块和断线恢复待实板。

```sh
mkdir -p .work
cc -std=c11 -Wall -Wextra -Werror examples/projects/sht30-display/check.c -o .work/sht30-check
.work/sht30-check
```

命令、时序和 CRC 见 [Sensirion SHT3x-DIS 数据手册](https://sensirion.com/media/documents/213E6A3B/63A5A569/Datasheet_SHT3x_DIS.pdf)。
