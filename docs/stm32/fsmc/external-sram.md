# 10 · 天空星用 FSMC 读写外部 SRAM

用 `volatile uint16_t` 访问 `0x60000000` 起的 1 MiB 外部 SRAM。固件分三步检查数据线、地址是否重叠和整片内容，遇到第一个读回差异就保存现场。

## 器件和接线

以裸天空星 STM32F407VET6、3.3 V 的 ISSI IS61WV51216BLL SRAM（512K×16）和两片 SN74LVC574A 为参考。这块核心板的 LQFP100 封装可用 DA0～DA15，但没有独立引出低 16 位地址组。连接方案采用地址/数据复用，两个触发器把低位地址从 DA 总线上取出。两片器件必须共地并按数据手册供电与去耦。使用短总线或转接板，按具体料号和封装从信号名对应脚号。

| 信号 | 天空星引脚 | 去向 |
|---|---|---|
| DA0～DA3 | PD14、PD15、PD0、PD1 | SRAM I/O0～I/O3，同时送进低位触发器 D0～D3 |
| DA4～DA7 | PE7～PE10 | SRAM I/O4～I/O7，同时送进低位触发器 D4～D7 |
| DA8～DA12 | PE11～PE15 | SRAM I/O8～I/O12，同时送进高位触发器 D0～D4 |
| DA13～DA15 | PD8、PD9、PD10 | SRAM I/O13～I/O15，同时送进高位触发器 D5～D7 |
| NADV | PB7 | 两片 SN74LVC574A 的 CLK。两片各自的低有效 OE 接地，Q 输出分别接 SRAM A0～A7、A8～A15 |
| A16～A18 | PD11、PD12、PD13 | SRAM A16～A18 |
| NE1、NOE、NWE | PD7、PD4、PD5 | SRAM CE#、OE#、WE# |
| NBL0、NBL1 | PE0、PE1 | SRAM LB#、UB# |

在 SRAM 端给 CE#、OE#、WE# 加到 3.3 V 的上拉，让它们在 STM32 复位时保持非选中。实验单独占用 NE1 和这些引脚，先断开并口 LCD、PB7 的 I²C 器件和同引脚的底板功能。

针脚与时序依据见 [STM32F407 数据手册的 FSMC 引脚表及图 56、57](https://www.st.com/resource/en/datasheet/stm32f407ve.pdf)、[ISSI IS61WV51216BLL](https://www.issi.com/WW/pdf/61-64WV51216.pdf)和 [TI SN74LVC574A](https://www.ti.com/lit/ds/symlink/sn74lvc574a.pdf)。

## 沿执行路径读代码

1. `fsmc_init()` 选异步、16 位、NE1。STM32 的 `MUXEN` 适用于 PSRAM 或 NOR 类型，本例选 PSRAM 总线模式，通过外置触发器接实际 SRAM。未开启 burst 和 NWAIT。
2. 地址阶段 DA0～DA15 提供低位地址。`NADV` 上升沿由触发器保存地址。接下来 DA 进入数据阶段，SRAM 通过触发器的输出继续得到原地址。
3. CPU 每次使用半字读写。外部 A0 对应 CPU 字节地址 bit1，`word=1` 映射到 `0x60000002`。物理容量是 `0x80000` 个半字，最后一项是 `0x600FFFFE`。
4. `sram_check()` 先在首地址做数据位行走，再在 `0` 与各个 `2^n` 的半字位置查别名。最后按地址生成图样，先整片写入，再从头逐个比对。异常时保存阶段、索引、预期值与实际值。

从默认 16 MHz HSI 开始。地址建立、保持、数据阶段和总线周转的配置值分别为 `6`、`4`、`18`、`4`。从这组保守配置开始，在板上看 NADV 上升沿时 DA 是否稳定、锁存输出是否先于读写选通过渡，以及写周期结束前数据是否已稳定。有关 `MUXEN` 和模式适用性见 [ST RM0090 的 FSMC 寄存器章节](https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)。

## 完整源码

<a href="../../../examples/stm32/fsmc/external-sram/main.c" download="main.c">下载 main.c</a> · <a href="../../../examples/stm32/fsmc/external-sram/sram_check.h" download="sram_check.h">下载自检逻辑</a> · <a href="../../../examples/stm32/fsmc/external-sram/check.c" download="check.c">下载主机检查</a> · <a href="../../../examples/stm32/fsmc/external-sram/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../../examples/stm32/fsmc/external-sram/main.c

<<< ../../../examples/stm32/fsmc/external-sram/sram_check.h{c}

## 构建和观察

```sh
cmake -S examples/stm32/fsmc/external-sram -B .work/sky-fsmc-sram -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-fsmc-sram
```

使用专用于本实验的外部 SRAM。固件覆盖整个测试区，已有内容会被改写。通过 SWD 加载 `.work/sky-fsmc-sram/sky-fsmc-sram.elf`，从复位运行。`checks_finished=1` 后看 `test_result`。`stage=SRAM_PASS` 表示三步通过；失败地址按 `0x60000000 + 2 × word` 算。检查期间避免在调试器里刷新外部 SRAM 内存窗口，以免额外访问混入波形。

## 练习

**预测题**　如果 A16 恒为 0，CPU 的 `0x60000000` 和 `0x60020000` 会落到不同位置吗？

<details><summary>核对预测</summary>

两处 CPU 地址相差 `0x20000` 字节，也就是 `0x10000` 个半字，只差外部 A16。A16 被固定时地址重叠，交替写入的图样会被读回检查发现。

</details>

**改动题**　把模拟器件模型中的 `word_mask` 改为忽略 A17，核对错误阶段。

<details><summary>核对改法</summary>

在 `check.c` 的别名用例把 `1U << 16U` 改为 `1U << 17U`。预期仍是 `SRAM_ADDRESS_ALIAS`。这个方法可找出地址重叠；地址线交换仍可能形成无重叠的地址排列，需要测量或核对接线。

</details>

## 验证记录

用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 交叉编译链接。反汇编确认 `ldrh` / `strh`，按两字节偏移。主机 512K 半字模型通过整片比对，并实测能报告数据位卡低、A16 别名和孤立内容错误。外置触发器时序、实际 SRAM 读写和负载下的信号完整性待天空星实板验证。

```sh
mkdir -p .work
cc -std=c11 -O2 -Wall -Wextra -Werror examples/stm32/fsmc/external-sram/check.c -o .work/kb-sram-check
.work/kb-sram-check
```
