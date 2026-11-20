# 12 · Monitor 等两个模块都报告进展才喂狗

A 和 B 各有周期。每个 400 ms 窗口内两项都推进才刷新 IWDG。持续按 PA0 用于模拟 B 卡住。

业务已经分别放进任务时，可看 [FreeRTOS Monitor 的实现](../../freertos/watchdog-monitor.md)。

## 板型和接线

天空星 PB2 提示 IWDG 复位，PA0 用于注入故障。连接供电和 SWD。

## 沿执行路径读代码

1. 启动保存复位原因，清标志，再开启独立看门狗。
2. 窗口到期时 Monitor 先结算上一轮。两份进展都齐才喂狗并开始新窗口。
3. 少一项就保留 missed 并锁住故障分支，硬件看门狗超时后复位。

## 完整源码

<a href="../../../examples/stm32/iwdg/health-window/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/iwdg/health-window/main.c

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。在仓库根目录执行以下命令，替换固件包路径。

```sh
cmake -S examples/stm32/iwdg/health-window -B .work/sky-iwdg-health -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-iwdg-health
```

通过 SWD 加载 `.work/sky-iwdg-health/sky-iwdg-health.elf` 后复位。正常时 refreshed 约每 400 ms 增加。按住覆盖一个完整窗口后，missed 指示 B 缺席。等待硬件复位并松键，PB2 会保留复位提示。

## 练习

**预测题**　A 报告三次、B 一次也没有，Monitor 会喂狗吗？

<details><summary>核对预测</summary>

需要 A 和 B 两个位。A 的重复报告无法补齐 B。

</details>

**改动题**　只短时间抑制 B，再观察下一次汇总。

<details><summary>核对改法</summary>

同一窗口内 B 若成功推进一次，到期仍可喂狗。

</details>

## 验证记录

已用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。接线及输出待实板核对。 [ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
