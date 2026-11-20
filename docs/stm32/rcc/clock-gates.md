# 天空星 RCC：GPIO 时钟使能和 AHB 分频

沿用 [01 GPIO 按键点灯](../gpio/button-led.md) 的 PA0 用户按键和 PB2 LED。先打开两个 GPIO 端口的时钟，再把 AHB 分频从 /1 改成 /2。在调试器里对照三份快照，就能看到哪一步改变了 GPIO 使能位，哪一步把 HCLK 从 16 MHz 改为 8 MHz。

需要切换系统时钟源时，继续看 [HSE、PLL 与 MCO 测频](hse-pll-mco.md)。

## 先看时钟经过哪些地方

```text
片内 HSI（标称 16 MHz）→ SYSCLK → AHB 分频 → HCLK
                                               ├─ Cortex-M4 内核与本例的 SysTick
                                               ├─ AHB1 外设时钟使能位 → GPIOA、GPIOB
                                               ├─ APB1 分频 → PCLK1
                                               └─ APB2 分频 → PCLK2
```

AHB 分频决定 HCLK。GPIOA、GPIOB 的时钟使能位决定各端口是否收到时钟。配置 PA0 的输入模式前先打开 GPIOA 的时钟，配置 PB2 的输出模式前先打开 GPIOB 的时钟。GPIO 端口时钟刚打开时，HAL 宏还会回读对应使能位，给时钟生效留出时间。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf) · [立创天空星 RCC 教程](https://wiki.lckfb.com/zh-hans/fdb/basic/rcc-clock-configuration.html)

## 完整示例

<a href="../../../examples/stm32/rcc/clock-gates/main.c" download="main.c">下载 RCC 示例 main.c</a>

<<< ../../../examples/stm32/rcc/clock-gates/main.c

## 顺着 main() 读

1. **记录复位后的基线。** 本例从复位启动，默认系统时钟来自片内 HSI。`HAL_Init()` 建立默认 SysTick 时基，`capture_rcc()` 拍下初始频率和 GPIO 使能位。
2. **打开两个端口的时钟，再配置引脚。** 两个使能宏分别置位 `RCC->AHB1ENR` 的 GPIOAEN、GPIOBEN。立即记录第二份快照，然后配置 PA0 输入和 PB2 输出。第二份快照应只改变两个使能位。
3. **只改 AHB 分频。** `ClockType` 只选 `RCC_CLOCKTYPE_HCLK`，所以 HAL 只更新 AHB 分频字段，SYSCLK 仍由 HSI 提供。APB1 和 APB2 分频仍为 /1，它们的频率跟着 HCLK 变成 8 MHz。HAL 更新 `SystemCoreClock` 并重设 SysTick 的重装值，毫秒节拍保持相同的标称周期。若时钟配置返回错误，可以在 `rcc_clock_status` 查看错误状态。
4. **回到点灯流程。** 主循环直接读取 PA0 并更新 PB2，按住应亮，松开应灭。这里可以下断点查看三份快照。继续运行后再观察按键和 LED。

## 构建并查看三份快照

使用与 GPIO 篇相同的 STM32CubeF4 V1.28.3、GNU Arm 编译器、CMake 和 Ninja。将固件包路径改成本机位置，在仓库根目录执行：

```sh
cmake -S examples/stm32/rcc/clock-gates -B .work/sky-rcc -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-rcc
```

使用用于实验的天空星 STM32F407VET6，按 [GPIO 篇的接线方式](../gpio/button-led.md#板型和引脚) 连接 SWD。下载 `.work/sky-rcc/sky-rcc-clock-gates.elf` 后复位，在主循环的 `HAL_GPIO_ReadPin()` 那一行下断点，运行至断点。Watch 窗口展开以下三个结构体。

| 观察变量 | `gpio_gate_bits` | `sysclk_hz` | `hclk_hz` | `pclk1_hz` / `pclk2_hz` | `systick_load` |
|---|---:|---:|---:|---:|---:|
| `rcc_before_gates` | `0x0` | 16 000 000 | 16 000 000 | 16 000 000 / 16 000 000 | 15 999 |
| `rcc_after_gates` | `0x3` | 16 000 000 | 16 000 000 | 16 000 000 / 16 000 000 | 15 999 |
| `rcc_after_divider` | `0x3` | 16 000 000 | 8 000 000 | 8 000 000 / 8 000 000 | 7 999 |

表中是从复位启动时的预期值。频率字段来自 HAL 按寄存器配置和 HSI 标称频率算出的数值。默认 SysTick 以 HCLK 计数，每毫秒需要 `HCLK / 1000` 个时钟周期，重装值再减 1。看完快照后撤掉断点，继续运行并验证按键点灯。

如果程序停在进入主循环之前，先查看 `rcc_clock_status`，再单步核对第 3 步的返回值。如果能进入主循环但 LED 无反应，先看 `rcc_after_gates.gpio_gate_bits` 是否是 `0x3`，再看 PA0 采样是否随按键变化。

## 练习

**预测题**　执行两个 GPIO 时钟使能宏后，HCLK、两个 PCLK 和 SysTick 重装值分别会怎样变化？

<details><summary>核对预测</summary>

它们都保持原值。变化发生在 AHB1 外设时钟使能寄存器，`gpio_gate_bits` 从 `0x0` 变为 `0x3`。时钟树的分频直到第 3 步才改变。

</details>

**改动题**　把 AHB 分频从 /2 改为 /4。重新编译前先算出第三份快照中的 HCLK、PCLK 和重装值。

<details><summary>核对改法</summary>

将 `clocks.AHBCLKDivider` 的初始值换成 `RCC_SYSCLK_DIV4`。第三份快照中 SYSCLK 仍是 16 MHz，HCLK、PCLK1 和 PCLK2 为 4 MHz，`systick_load` 为 3 999。预期继续运行时仍可按键点灯。

</details>

## 验证记录

已按 STM32F407 参考手册与 STM32CubeF4 V1.28.3 中的 HAL 源码核对时钟设置和预期值。使用 GNU Arm 14.3.1 编译链接成功，生成 `sky-rcc-clock-gates.elf`，/4 练习通过 Arm 编译器检查。快照读数和按键点灯待实板核对。
