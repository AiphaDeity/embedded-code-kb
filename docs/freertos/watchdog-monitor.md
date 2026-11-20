# FreeRTOS 汇总任务进展后刷新独立看门狗

两个业务任务按各自周期工作，完成一轮后向事件组报告。Monitor 每 400 ms 结算一次。两项都完成才刷新 IWDG；缺一项时，记录缺项并锁定等待硬件复位。

## 板型和操作

使用天空星 STM32F407VET6，接供电与 SWD。PA0 板载按键模拟 B 无法完成本轮工作，PB2 LED 提示上一次复位来自 IWDG。

## 沿执行路径读代码

1. `main()` 从 `RCC->CSR` 保存复位标志再清除，创建事件组、两项业务任务和 Monitor。最后启动 IWDG 和调度器。IWDG 按标称 32 kHz LSI 估算约 2 秒，板端计时会受 LSI 频率影响。
2. `run_worker()` 在自己的周期内完成工作后置位。B 如果看到 PA0 按下或 `inject_missing_b` 非零，这一轮就不置位。
3. `monitor()` 用 `vTaskDelayUntil()` 到固定窗口边界。`xEventGroupClearBits()` 同一次调用取清除前的位并清除它们，重复置 A 位无法补齐 B。
4. 两项齐全时 Monitor 刷新 IWDG。窗口缺项时保存 `missing_tasks`，设置 `monitor_latched` 并挂起。业务任务继续调度；即使 B 恢复，Monitor 在本次启动中也不会回到刷新路径。

事件组返回值与硬件复位原因见 [FreeRTOS 事件组参考](https://www.freertos.org/media/2018/FreeRTOS_Reference_Manual_V10.0.0.pdf)和 [ST RM0090](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)。

## 完整源码

<a href="../../examples/freertos/watchdog-monitor/main.c" download="main.c">下载 main.c</a> · <a href="../../examples/freertos/watchdog-monitor/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../examples/freertos/watchdog-monitor/main.c

## 构建与观察

```sh
cmake -S examples/freertos/watchdog-monitor -B .work/sky-freertos-watchdog-monitor -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-freertos-watchdog-monitor
```

通过 SWD 加载本目录生成的 `sky-freertos-watchdog-monitor.elf`，从复位运行。正常时 `a_completed`、`b_completed` 和 `feeds` 持续增加。按住 PA0 覆盖一个完整窗口，等待复位后松开。预期 PB2 点亮，`reset_flags` 中可看到 `IWDGRSTF`。做真实超时测试时，让固件连续运行，并确认调试器没有冻结 IWDG。

## 练习

**预测题**　B 在窗口前半段已报告一次，随后停止，当前窗口还会刷新吗？

<details><summary>核对预测</summary>

如果 A 也报告过，当前窗口可以刷新一次。结算时两位清掉；下一窗口需要重新收到两份进展。B 一直缺席，下一次结算就会触发锁定。

</details>

**改动题**　通过调试器把 `inject_missing_b` 设为 1，看到缺项后设回 0，比较 B 的完成次数和喂狗次数。

<details><summary>核对改法</summary>

缺项时 `missing_tasks=2`、`monitor_latched=1`。若在硬件复位前解除注入，B 可以再次完成任务，但当前启动的 `feeds` 仍停在锁定时的值。复位后调试注入量会随 BSS 清零。

</details>

## 验证记录

用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。在 QEMU Cortex-M4 上运行时，正常阶段刷新 10 次；注入 B 缺席后，最后一个已有进展的窗口刷新一次，随后 `missing_tasks=2` 并锁定。解除注入后 B 计数从 31 增至 63，刷新计数保持 11。实际 IWDG 倒计时、复位原因和 LED 待实板。
