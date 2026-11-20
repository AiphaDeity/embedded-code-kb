# 01 · 按键稳定后翻灯，主循环继续跑

PA0 稳定按下 20 ms 后翻转 PB2 一次。长按保持灯的状态，后台计数继续更新。

## 板型和接线

天空星板载 PA0 按键与 PB2 LED，连接供电和 SWD。

## 沿执行路径读代码

1. 每次采样更新候选电平，只有稳定时间到才交出一个边沿。
2. 主循环只在按下事件时翻灯，松开时记录释放。
3. 等消抖期间，其他逻辑仍能每 100 ms 更新 heartbeat。

## 完整源码

<a href="../../../examples/stm32/gpio/debounced-button/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/gpio/debounced-button/main.c

消抖逻辑和主机测试见[按键稳定时间](../../c/debounce.md)。

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。在仓库根目录执行以下命令，替换固件包路径。

```sh
cmake -S examples/stm32/gpio/debounced-button -B .work/sky-gpio-debounce -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-gpio-debounce
```

通过 SWD 加载 `.work/sky-gpio-debounce/sky-gpio-debounce.elf` 后复位。每次有效按下 presses 加一，长按期间 heartbeat 可继续增加。

## 练习

**预测题**　高、低、高的抖动从哪次开始计时？

<details><summary>核对预测</summary>

从最后一次观察到的候选变化开始计时。

</details>

**改动题**　把 20 ms 改成 50 ms，试一次短于 50 ms 的按压。

<details><summary>核对改法</summary>

候选值来不及成为稳定值，presses 保持原值。

</details>

## 验证记录

已用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。接线及输出待实板核对。 [ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
