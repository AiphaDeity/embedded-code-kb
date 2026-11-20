# 02 · 天空星从 HSI 切到 HSE + PLL，并用 MCO 测频

天空星从默认 HSI 启动。先让 8 MHz 外部晶振与 PLL 就绪，再把 SYSCLK 切到 48 MHz。PA8 输出可用示波器核对的时钟。

## 板型和接线

使用板上有 8 MHz HSE 晶振的天空星 STM32F407VET6，按核心板规格供电。示波器用高阻探头接 PA8/MCO1，地夹接板上 GND。保持 PA8 上只有测量负载。板型与时钟输出来源见 [天空星核心板资料](https://wiki.lckfb.com/zh-hans/fdb/skystar.html)和 [STM32F407 RM0090，RCC 章节](https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)。

## 沿执行路径读代码

1. `HAL_Init()` 先用 HSI 建立 SysTick，再记录 `after_reset`。`HAL_RCC_MCOConfig()` 提前选定 PA8 输出源。
2. `HAL_RCC_OscConfig()` 打开 HSE，再锁定 PLL。等这一步返回后，`after_pll_lock.cr` 应看到 `HSERDY` 和 `PLLRDY`，此时 SYSCLK 仍走 HSI。启动失败时停在这里并保留返回值。
3. `HAL_RCC_ClockConfig()` 提高 Flash 等待周期，设置总线分频，切换 SYSCLK 并更新 SysTick。成功后 `after_switch.cfgr` 的 `SWS` 表示 PLL 已接管。
4. 主循环每次更新 `heartbeat` 再延时。它用来观察切换之后毫秒时基是否仍在推进。

`8 MHz ÷ 8 × 192 = 192 MHz` 是 VCO 频率，再除以 `PLLP=4` 得到 48 MHz SYSCLK。AHB `/1` 得 48 MHz HCLK，APB1 `/2` 得 24 MHz PCLK1，APB2 `/1` 得 48 MHz PCLK2。1 ms SysTick 使用 `48000 - 1` 作为重装值。

## 完整源码

<a href="../../../examples/stm32/rcc/hse-pll-mco/main.c" download="main.c">下载 main.c</a> · <a href="../../../examples/stm32/rcc/hse-pll-mco/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../../examples/stm32/rcc/hse-pll-mco/main.c

## 构建与观察

```sh
cmake -S examples/stm32/rcc/hse-pll-mco -B .work/sky-rcc-hse-pll -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-rcc-hse-pll
```

SWD 加载 `.work/sky-rcc-hse-pll/sky-rcc-hse-pll.elf`。核对三个快照，再让固件运行。观察 PA8 输出约 8 MHz，`heartbeat` 约每秒增加一次。HAL 显示的是按配置计算的标称频率，PA8 的实际输出以仪表读数为准。

## 练习

**预测题**　`PLLRDY` 已置位，程序还没执行时钟切换。这时内核和 PA8 分别用什么时钟？

<details><summary>核对预测</summary>

默认构建下内核还用标称 16 MHz 的 HSI。PA8 已选 HSE，HSE 就绪后输出约 8 MHz。查看 `CR` 可核对 PLL 是否就绪，查看 `CFGR.SWS` 才知道 SYSCLK 当前来源。

</details>

**改动题**　构建时加 `-DSKY_MCO_PLL=ON`，从 PA8 观察 PLL 输出。

<details><summary>核对改法</summary>

用另一构建目录，例如 `.work/sky-rcc-hse-pll-mco12`，并保留原命令里的工具链参数。选项将 PA8 改为 PLL `/4`，因此测量值约 12 MHz，`after_switch.sysclk_hz` 仍预计为 48 000 000 Hz。

</details>

## 验证记录

用 STM32CubeF4 V1.28.3 与 GNU Arm 14.3.1 编译链接了 HSE 直出和 PLL 分频输出两种配置。寄存器现场值、PA8 波形与晶振启动时序待实板核对。
