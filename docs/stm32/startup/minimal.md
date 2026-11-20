# STM32 从复位走到最小主循环

用天空星 STM32F407VET6 的 PB2 LED 和板上 HSI，观察复位后 C 变量如何就位，以及 `main()` 怎样依次检查内存、初始化 GPIO、启动 SysTick。预期 PB2 每 500 ms 翻转一次，无需外接器件。

## 从复位入口读到 main

1. 用 SWD 加载 ELF，复位后在 `Reset_Handler` 停住。向量表在 Flash 起点 `0x08000000`，第一项为初始栈顶 `0x20020000`，第二项指向带 Thumb 位的复位入口。核对生成的 map 中的 `.isr_vector` 和 `_estack`。
2. STM32CubeF4 的 `startup_stm32f407xx.s` 先调用 `SystemInit()`，再把 `_sidata` 的 Flash 初值复制到 RAM 的 `[_sdata, _edata)`，把 `[_sbss, _ebss)` 清零。随后调用 `__libc_init_array()` 和 `main()`。SDK 中的启动文件位于 `Drivers/CMSIS/Device/ST/STM32F4xx/Source/Templates/gcc/`。
3. `main()` 读取 `.data` 中的 `boot_cookie` 和 `.bss` 中的 `zero_cookie`。两个值分别应为 `0x1234ABCD` 和 `0`，否则 `startup_stage` 留在 `0xE1`，程序停止在 `stop_at()`。
4. `led_init()` 先预置 PB2 低电平再设置输出模式。SysTick 启动成功后 `startup_stage` 为 3。中断只增加 `tick_ms`，主循环比较无符号时间差，满 500 ms 才翻转灯并增加 `led_toggles`。SysTick 配置失败时状态为 `0xE2`。

## 完整源码

<a href="../../../examples/stm32/startup/minimal/main.c" download="main.c">下载 main.c</a> · <a href="../../../examples/stm32/startup/minimal/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a> · <a href="../../../examples/stm32/common/sky-cmsis.cmake" download="sky-cmsis.cmake">下载 CMSIS 构建规则</a> · <a href="../../../examples/stm32/common/stm32f407ve.ld" download="stm32f407ve.ld">下载链接脚本</a>

<<< ../../../examples/stm32/startup/minimal/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。在仓库根目录执行，固件包路径换成本机位置。

```sh
cmake -S examples/stm32/startup/minimal -B .work/sky-minimal-startup -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-minimal-startup
arm-none-eabi-objdump -h .work/sky-minimal-startup/sky-minimal-startup.elf
arm-none-eabi-objdump -s -j .isr_vector .work/sky-minimal-startup/sky-minimal-startup.elf
```

查看同目录的 `sky-minimal-startup.map`。对 `.data` 比较 VMA 和 LMA，前者是变量运行时的 RAM 地址，后者是上电需要复制的 Flash 地址。继续运行时，通过 Watch 对照 `tick_ms` 和 `led_toggles`。使用 SWD 接天空星，板载 PB2 为高电平点亮。

## 练习

**预测题**　如果在 `main()` 第一条判断前把 `zero_cookie` 改为 1，下一段去哪？

<details><summary>核对预测</summary>

进入 `stop_at(0xE1U)`，状态为 `0xE1`。GPIO 和节拍还没有初始化，`led_toggles` 保持 0。

</details>

**改动题**　把 LED 的翻转间隔改成 250 ms，保持系统节拍为 1 ms。

<details><summary>核对改法</summary>

只改 `PERIOD_MS` 为 250。对比两次暂停之间的 `tick_ms` 和 `led_toggles` 增量，大约每增加 250 tick，多翻转一次。

</details>

## 验证记录

GNU Arm 14.3.1 编译链接通过。map 中 `.data` VMA 为 `0x20000000`、LMA 为 `0x08000390`。QEMU 复位入口运行前写入两个脏值，到 `main()` 入口分别恢复为 `0x1234ABCD` 和 `0`。运行到 3104 tick 时记录 6 次翻转。在 `main()` 入口改坏 `boot_cookie` 后停在 `0xE1`，节拍和翻转计数仍为 0。
