# 01 · 天空星 GPIO 开漏输出并回读实际电平

让 PA1 在主动拉低和释放之间切换。把输出锁存器 `ODR` 和实际引脚 `IDR` 放在同一份快照里，观察开漏释放后外部电路仍能把线拉低。

## 板型和接线

使用天空星 STM32F407VET6。用一个 10 kΩ 电阻把 PA1 上拉到板上 3.3 V。常开按键接在 PA1 与 GND 之间，按下时把线拉低。万用表或示波器对板上 GND 测 PA1，测试时让 PA1 只连接上述电路。开漏输出电路见 [ST AN4899](https://www.st.com/resource/en/application_note/an4899-stm32-microcontroller-gpio-hardware-settings-and-lowpower-consumption-stmicroelectronics.pdf)。

## 沿执行路径读代码

1. `main()` 先把 PA1 的输出锁存器设为 1，再切到 `GPIO_MODE_OUTPUT_OD`，引脚以释放状态开始。
2. 主循环写 0 以后由 STM32 主动拉低。等 10 ms 后，`sample_pin()` 记录 `ODR` 与 `HAL_GPIO_ReadPin()` 读取的 `IDR`。
3. 主循环写 1 释放引脚。松开外部按键时电阻拉高，按住时按钮维持低电平。两个阶段各持续约半秒，方便观察。

## 完整源码

<a href="../../../examples/stm32/gpio/open-drain-readback/main.c" download="main.c">下载 main.c</a> · <a href="../../../examples/stm32/gpio/open-drain-readback/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../../examples/stm32/gpio/open-drain-readback/main.c

## 构建与观察

```sh
cmake -S examples/stm32/gpio/open-drain-readback -B .work/sky-gpio-open-drain -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-gpio-open-drain
```

通过 SWD 加载 `.work/sky-gpio-open-drain/sky-gpio-open-drain.elf`，从复位运行。

| 快照 | 外部按键 | `output_latch` | `input_level` |
|---|---|---:|---:|
| `after_drive_low` | 任意 | 0 | 0 |
| `after_release` | 松开 | 1 | 1 |
| `after_release` | 按住 | 1 | 0 |

## 练习

**预测题**　释放阶段按住按钮，读 `GPIOA->ODR` 能看出按钮状态吗？

<details><summary>核对预测</summary>

`ODR` 第 1 位持续为 1，表示开漏输出释放。要知道线被外部拉到了哪里，用 `IDR` 或直接量 PA1。

</details>

**改动题**　将释放阶段改为更长，观察示波器上的占空比例。

<details><summary>核对改法</summary>

将 `after_release` 后的等待由 490 ms 改为 990 ms，释放约 1 秒，拉低仍约半秒。松开按键时，高电平时间约占三分之二。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译链接。PA1 电平、拉低与释放两个阶段的回读待实板记录。
