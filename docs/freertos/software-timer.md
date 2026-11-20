# FreeRTOS 软件定时器交接周期事件

创建一个每 500 ms 自动重载的软件定时器。定时器服务任务只发通知，消费者任务收到后更新 LED。

## 执行顺序

1. 创建消费者和定时器，再用 `xTimerStart()` 提交启动命令。
2. 到期回调运行在定时器服务任务，使用任务版本的通知接口。
3. 消费者阻塞等通知，取出后更新 `handled` 并翻转 LED。

## 完整源码

<a href="../../examples/freertos/software-timer/main.c" download="main.c">下载示例 main.c</a>

<<< ../../examples/freertos/software-timer/main.c

## 构建与观察

使用天空星 STM32F407VET6 的板载 PB2 LED 和 SWD 调试器。工程复用天空星链接脚本，并使用 STM32CubeF4 V1.28.3 固件包内的 FreeRTOS 内核。准备 GNU Arm、CMake、Ninja，将固件包路径换成本机位置：

```sh
cmake -S examples/freertos/software-timer -B .work/sky-software-timer -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-software-timer
```

在实验板上通过 SWD 加载 `.work/sky-software-timer/sky-software-timer.elf` 后复位运行。

`fired` 每秒约增加两次，`handled` 应跟随。每次翻转间隔约 500 ms。

## 练习

**预测题**　软件定时器回调可否调用 `vTaskDelay()` 实现第二段等待？

<details><summary>核对预测</summary>

回调占用共享的定时器服务任务，阻塞会延迟其他回调。把等待和耗时操作交给各自任务。

</details>

**改动题**　将自动重载改成 `pdFALSE`，观察回调次数。

<details><summary>核对改法</summary>

成功启动一次后只执行一次回调，`fired` 与 `handled` 应各到 1。

</details>

## 验证记录

已使用 GNU Arm 14.3.1 与 STM32CubeF4 V1.28.3 编译链接。QEMU `netduinoplus2` 的 Cortex-M4 运行本 ELF 时，`fired` 和 `handled` 均为 24。天空星的实际周期与 LED 待验证。
