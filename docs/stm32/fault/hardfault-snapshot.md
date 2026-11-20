# STM32 HardFault 后读取堆栈 PC 并定位源码

通过 SWD 运行这个独立练习固件。它会执行一条未定义指令，再停在 `fault_halt()`。用快照中的 PC 找到触发异常的那一行。

## 沿执行路径读代码

1. `main()` 保持独立 UsageFault 关闭，然后执行 `UDF`。该故障升级到 HardFault，`HFSR.FORCED` 记录升级，`CFSR.UNDEFINSTR` 记录原因。
2. CPU 压入基本异常帧并进入 `HardFault_Handler()`。裸汇编在 C 函数改动栈前，按 `EXC_RETURN` 第 2 位取 MSP 或 PSP，将帧地址和 `EXC_RETURN` 交给 `fault_capture()`。
3. `fault_capture()` 先保存故障位，再检查帧类型、压栈错误和地址范围。只有可读的基本帧才取其中的 PC、LR 和 xPSR；`has_basic_frame=0` 时用故障位继续分析。
4. 写入 `complete` 后停住。调试器此时的当前 PC 在处理函数里，`fault_report.frame.pc` 才是本次练习触发 `UDF` 的位置。

本例的汇编入口和栈范围按天空星 STM32F407VET6 构建，只演示普通 SRAM 中的基本帧。异常帧和 `CFSR` 位定义参阅 [ST Cortex-M4 编程手册 PM0214](https://www.st.com/resource/en/programming_manual/pm0214-stm32-cortexm4-mcus-and-mpus-programming-manual-stmicroelectronics.pdf)。

## 完整源码

<a href="../../../examples/stm32/fault/hardfault-snapshot/main.c" download="main.c">下载 main.c</a> · <a href="../../../examples/stm32/fault/hardfault-snapshot/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../../examples/stm32/fault/hardfault-snapshot/main.c

## 构建和定位

```sh
cmake -S examples/stm32/fault/hardfault-snapshot -B .work/sky-hardfault-snapshot -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-hardfault-snapshot
```

用 SWD 加载该目录中的 `sky-hardfault-snapshot.elf`。在 `fault_halt()` 设断点，再从复位运行。可在 GDB 控制台查看。

```text
p/x fault_report
x/i fault_report.frame.pc
```

预期 `complete=1`、`has_basic_frame=1`、`hfsr=0x40000000`、`cfsr=0x00010000`。也可复制报告中的 PC，并用**实际下载的同一份 ELF** 查询源码位置。

```sh
arm-none-eabi-addr2line -e .work/sky-hardfault-snapshot/sky-hardfault-snapshot.elf -f 0x080001cc
```

`0x080001cc` 是本次构建的示例地址，工程改动后按快照重取。`MMFAR` 和 `BFAR` 仅在 `CFSR` 对应有效位置位时用于分析。

## 练习

**预测题**　报告的 `HFSR.FORCED` 置位后，从哪个字段知道这次为什么进入 HardFault？

<details><summary>核对预测</summary>

从 `CFSR.UNDEFINSTR` 知道触发的是未定义指令。再用已保存的 `frame.pc` 找到对应的 `UDF`。

</details>

**改动题**　把 `udf #0` 改成 `udf #1`，重新定位。

<details><summary>核对改法</summary>

重新编译下载，故障类别仍为 `UNDEFINSTR`。用 `x/i fault_report.frame.pc` 可看到立即数变成 1。对比的是本次栈中 PC 和本次 ELF。

</details>

## 验证记录

已用 GNU Arm 14.3.1 交叉编译链接。向量表第 4 项指向自定义 HardFault 入口。反汇编核对 MSP / PSP 选择，`addr2line` 定位到 `trigger_fault()`。

在 QEMU `netduinoplus2` 的 Cortex-M4 上实际运行此 ELF。到达 `fault_halt()` 时读到 `EXC_RETURN=0xFFFFFFF9`、`HFSR=0x40000000`、`CFSR=0x00010000`，堆栈中的 PC 为 `0x080001CC`，对应 `UDF #0`。天空星实板断点与现场待验证。
