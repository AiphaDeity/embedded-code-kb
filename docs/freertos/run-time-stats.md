# FreeRTOS 用 TIM2 记录任务运行时间

两个任务每 20 ms 分别忙等约 1 ms 和 3 ms。监测任务每 500 ms 读取一次 FreeRTOS 的运行时间计数，调试器可以比较轻负载、重负载和 Idle 在同一窗口中的运行时间。

## 沿执行路径读代码

1. 此目标编译时打开 `SKY_USE_RUN_TIME_STATS`。内核启动调度前调用 `run_time_clock_init()`。天空星默认 HSI 和总线不分频时，TIM2 的 16 MHz 分频到 10 kHz。每个计数对应 100 µs，计数器自己运行，无需中断。
2. `load_task()` 忙等指定数目的 TIM2 计数，然后通过 `vTaskDelayUntil()` 等待下一轮。内核在切换任务时读 `TIM2->CNT`，把这段时间累加给被切出的任务。
3. `monitor_task()` 调用 `uxTaskGetSystemState()` 取最多四项，按句柄识别轻负载、重负载和 Idle。第一次只留基准，之后用相邻快照的差填 `window`。快照失败时重新取基准。TIM2 回绕时跳过跨界窗口。

窗口内的运行占比可用 `window.heavy / window.clock` 估算。轻重任务的忙等长度按理想时序约占 5% 和 15%，抢占、采样和中断都会影响观察值。FreeRTOS 这份版本在任务切出时累计，窗口边缘尚未切出的片段可能落到下一个窗口。此例用来诊断任务占用，计数也可能包含挂在该任务上下文上的中断时间。10 kHz 下 32 位 TIM2 约 4.97 天回绕，观察长期负载时应考虑计数来源和内核版本。[FreeRTOS API 参考](https://freertos.org/media/2025/FreeRTOS_Reference_Manual_V8.2.1.pdf) · [STM32 RM0090](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)

## 完整源码

<a href="../../examples/freertos/run-time-stats/main.c" download="main.c">下载 main.c</a> · <a href="../../examples/freertos/run-time-stats/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a> · <a href="../../examples/freertos/common/FreeRTOSConfig.h" download="FreeRTOSConfig.h">下载内核配置</a>

<<< ../../examples/freertos/run-time-stats/main.c

## 构建和观察

天空星 STM32F407VET6 接供电和 SWD。使用 GNU Arm、CMake、Ninja、STM32CubeF4 V1.28.3，替换固件包路径。

```sh
cmake -S examples/freertos/run-time-stats -B .work/sky-freertos-run-time-stats -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-freertos-run-time-stats
```

加载 `sky-freertos-run-time-stats.elf` 后至少运行 1 秒，再看 `windows`、`window.clock`、`window.light`、`window.heavy`、`window.idle`。在多个窗口里对比趋势。复用到已经改过总线分频的工程时，按实际 TIM2 输入时钟计算 PSC，并确认 TIM2 可独占使用。

## 练习

**预测题**　任务在 `vTaskDelayUntil()` 阻塞的时间计给谁？

<details><summary>核对预测</summary>

内核把计数记给实际得到 CPU 的任务。如果其他可运行任务也已经阻塞，Idle 获得这一段时间。

</details>

**改动题**　把重负载常量从 30 改成 10，比较两个任务的窗口计数。

<details><summary>核对改法</summary>

两个任务周期和忙等长度相同，多个窗口的 `window.light` 和 `window.heavy` 应接近。单窗口仍会有排序、切换与量化造成的差异。

</details>

## 验证记录

GNU Arm 14.3.1 与随 STM32CubeF4 V1.28.3 提供的 FreeRTOS V10.3.1 编译链接通过。QEMU 一次窗口记录为总计 30184、轻负载 296、重负载 961、Idle 28915，采样错误为 0。仿真的定时器速率与板上时钟不同，这组值用于核对计数路径和负载差。
