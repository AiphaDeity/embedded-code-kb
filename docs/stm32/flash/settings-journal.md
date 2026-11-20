# 把 LED 参数追加到片内 Flash，复位后恢复

PA0 每次稳定按下都会保存一个新翻转间隔。参数、CRC、提交标记按顺序写入，启动选择最后一条完整记录。

## 板型和接线

使用专用 STM32F407VET6 实验板和正常 3.3 V 供电。链接限制在 0x08000000～0x0805FFFF，扇区 7（0x08060000，128 KiB）留作实验参数区。

## 沿执行路径读代码

1. 扫描时选择最后一条通过提交标记、范围和 CRC 检查的记录，空白时用 1000 ms。
2. 按键后在最后一个使用过的槽之后追加，先写参数及 CRC，再写提交标记。
3. 按启动规则读回，更新本轮运行参数。空间用尽时报告 SAVE_FULL，旧记录仍可恢复。

## 完整源码

<a href="../../../examples/stm32/flash/settings-journal/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/flash/settings-journal/main.c

<a href="../../../examples/stm32/flash/settings-journal/journal.h" download="journal.h">下载扫描逻辑</a> · <a href="../../../examples/c/common/crc32.h" download="crc32.h">下载 CRC-32</a> · <a href="../../../examples/stm32/flash/settings-journal/check.c" download="check.c">下载主机检查</a>

<<< ../../../examples/stm32/flash/settings-journal/journal.h{c}

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。在仓库根目录执行以下命令，替换固件包路径。

```sh
cmake -S examples/stm32/flash/settings-journal -B .work/sky-flash-settings -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-flash-settings
```

通过 SWD 加载 `.work/sky-flash-settings/sky-flash-settings.elf` 后复位。空白时 current_period_ms 为 1000，连续按键两次应变成 100、200。用复位键重启，核对参数与 next_slot。下载新固件时保留参数扇区。

## 练习

**预测题**　写完 CRC、尚未写提交标记时断电，下一次恢复哪条？

<details><summary>核对预测</summary>

恢复上一条有效记录。半写槽已被使用，追加从它之后开始。

</details>

**改动题**　在主机检查里让槽 3 的 CRC 与参数 700 匹配。

<details><summary>核对改法</summary>

扫描会返回 700，需要同步更新后续的预期。

</details>

## 验证记录

从仓库根目录在 Windows 运行主机检查，用同一份扫描逻辑验证恢复路径。

```powershell
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/stm32/flash/settings-journal/check.c -o .work/flash-journal-check.exe
& ./.work/flash-journal-check.exe
```

已编译链接目标。Windows GCC 主机检查覆盖擦除态、断写、CRC 错误、恢复最新记录及满扇区。实际 Flash 写入和掉电待验证。 [ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
