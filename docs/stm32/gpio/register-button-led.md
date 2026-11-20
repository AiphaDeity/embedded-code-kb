# 01 · 用 GPIO 寄存器实现按键点灯

在天空星 STM32F407VET6 上，用 PA0 读板载按键，用 PB2 驱动板载 LED。按住亮，松开灭。可沿两个版本对照 GPIO 如何配置和写入。[HAL 按键点灯版](button-led.md)

## 板型和接线

给核心板供电，并接 SWD 下载。PA0 使用核心板已有下拉，PB2 使用板载灯。

## 沿执行路径读代码

1. 对照 `__HAL_RCC_GPIOx_CLK_ENABLE()`，先使能 AHB1 上的两个 GPIO 时钟。
2. 对照 `HAL_GPIO_Init()`，查看 PA0 与 PB2 在 `MODER`、`PUPDR` 和 `OTYPER` 中各占哪些位。
3. 主循环从 `IDR` 取 PA0。当它为高时写 `BSRR` 低半字，当它为低时写高半字。两次操作都只影响 PB2。

## 完整源码

<a href="../../../examples/stm32/gpio/register-button-led/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/gpio/register-button-led/main.c

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3，替换固件包路径后在仓库根目录执行。

```sh
cmake -S examples/stm32/gpio/register-button-led -B .work/sky-gpio-register-button -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-gpio-register-button
```

通过 SWD 加载 `.work/sky-gpio-register-button/sky-gpio-register-button.elf` 后复位运行。调试器中观察 `GPIOA->IDR` 的第 0 位。按住为 1，松开为 0，PB2 每轮按该输入更新。

## 练习

**预测题**　置位和复位都写 `BSRR`，为什么 LED 状态不同？

<details><summary>核对预测</summary>

`BSRR` 低半字控制置位，高半字控制复位。对 PB2 写 `1 << 2` 与写 `1 << 18` 执行不同命令。

</details>

**改动题**　让松开时点亮，按住时熄灭。

<details><summary>核对改法</summary>

交换最后一行三元表达式里的两个命令，保持输入配置和采样步骤不变。

</details>

## 验证记录

目标用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。接线与观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
