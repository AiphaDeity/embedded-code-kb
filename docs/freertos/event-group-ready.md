# FreeRTOS 事件组等两个模块就绪

配置服务在 100 ms 后就绪，设备服务在 750 ms 后就绪。等待任务只有看到两位都置位才点亮 PB2。两项耗时用任务延时模拟，可直接在天空星核心板上复现。

## 沿执行路径读代码

1. `main()` 创建事件组和三个任务。优先执行的等待任务先进入 `xEventGroupWaitBits()`。
2. 两个服务各持有一个就绪位。完成后设置该位，等待方保留这些位供下次判断。
3. 等待方每到 300 ms 记一次超时。两位凑齐时退出等待，点亮 LED 并保留就绪状态。

## 接线和源码

使用天空星 STM32F407VET6 和 PB2 板载 LED，连接供电和 SWD，不需要外接模块。

<a href="../../examples/freertos/event-group-ready/main.c" download="main.c">下载示例 main.c</a>

<<< ../../examples/freertos/event-group-ready/main.c

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3，从仓库根目录执行。

```sh
cmake -S examples/freertos/event-group-ready -B .work/sky-freertos-event-group -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-freertos-event-group
```

通过 SWD 加载 `.work/sky-freertos-event-group/sky-freertos-event-group.elf` 后复位运行。预计先有两次超时，`last_timeout_missing` 只包含 `DEVICE_READY`，约 750 ms 时 LED 点亮，`application_ready` 变为真。观察时间从调度器启动起算。

## 练习

**预测题**　先设置 `CONFIG_READY`，两次超时后该位还在吗？

<details><summary>核对预测</summary>

还在。`xEventGroupWaitBits()` 的退出清位参数为 `pdFALSE`。

</details>

**改动题**　把设备服务延时从 750 ms 改成 250 ms。

<details><summary>核对改法</summary>

两位预计在首次 300 ms 超时前凑齐，LED 提前点亮，`timeouts` 保持 0。

</details>

## 验证记录

已使用 GNU Arm 14.3.1 与 STM32CubeF4 V1.28.3 编译链接。QEMU `netduinoplus2` 的 Cortex-M4 运行本 ELF 后，`timeouts=2`、`last_timeout_missing=0x02`、`application_ready=1`。天空星上的调度与 LED 待验证。
