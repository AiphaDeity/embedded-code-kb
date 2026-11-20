# RTC 闹钟从 STOP 唤醒并恢复时钟

天空星进入 STOP，由约 5 秒后的 RTC Alarm A 唤醒，恢复 84 MHz 并翻转 PB2。

## 板型和接线

连接供电和 SWD。实验用 LSI。备份域已选其他 RTC 时钟源时，程序在 phase 2 停下并保留该配置。

## 沿执行路径读代码

1. PLL 建立 84 MHz，RTC 用备份标记判断是否需要写入初始日历。
2. 读 RTC 时间和日期，计算约 5 秒后的时分秒。闹钟不比较日期，跨午夜也能触发。
3. 暂停 SysTick 并进入 STOP。闹钟挂起可唤醒内核，主循环解除掩码后回调计数。
4. 恢复时间基准与 PLL，记录 SystemCoreClock，再翻灯并设置下一次闹钟。

## 完整源码

<a href="../../../examples/stm32/rtc/stop-alarm/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/rtc/stop-alarm/main.c

<a href="../../../examples/stm32/common/clock84.h" download="clock84.h">下载 PLL 时钟配置</a>

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。在仓库根目录执行以下命令，替换固件包路径。

```sh
cmake -S examples/stm32/rtc/stop-alarm -B .work/sky-rtc-stop-alarm -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-rtc-stop-alarm
```

通过 SWD 加载 `.work/sky-rtc-stop-alarm/sky-rtc-stop-alarm.elf` 后复位。预计 LED 每约 5 秒翻转，alarms 增长，resumed_clock_hz 为 84000000。测整板电流时包含 LED、稳压器与调试器等外设带来的负载。

## 练习

**预测题**　23:59:55 设置 5 秒后的闹钟，会得到什么？

<details><summary>核对预测</summary>

次日 00:00:00。RTC 更新日期，Alarm A 只比较时分秒。

</details>

**改动题**　把 ALARM_DELAY 改成 2，观察翻转间隔。

<details><summary>核对改法</summary>

每轮约 2 秒，实际误差受 LSI 影响。

</details>

## 验证记录

已用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。接线及输出待实板核对。 [ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
