# FreeRTOS 任务通知与周期采样

生产任务每 250 ms 发一次通知，消费任务阻塞等待，收到后累加取到的事件数并翻转 PB2。

## 执行顺序

1. 先创建消费者并保存任务句柄，再创建生产者，最后启动调度器。
2. 生产者使用 `vTaskDelayUntil()` 保持相对固定的周期。
3. 消费者用 `ulTaskNotifyTake(pdTRUE, portMAX_DELAY)` 取出当前累计数并清零。

## 完整源码

<a href="../../examples/freertos/task-notification/main.c" download="main.c">下载示例 main.c</a>

<<< ../../examples/freertos/task-notification/main.c

## 构建与观察

使用天空星 STM32F407VET6 的板载 PB2 LED 和 SWD 调试器。工程复用天空星链接脚本，并使用 STM32CubeF4 V1.28.3 固件包内的 FreeRTOS 内核。准备 GNU Arm、CMake、Ninja，将固件包路径换成本机位置：

```sh
cmake -S examples/freertos/task-notification -B .work/sky-task-notification -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-task-notification
```

在实验板上通过 SWD 加载 `.work/sky-task-notification/sky-task-notification.elf` 后复位运行。

`notified` 与 `consumed` 随时间增长，稳定运行时两者若暂时不同，等待消费者处理即可。PB2 每收到一批通知翻转一次。

## 练习

**预测题**　若消费任务来不及运行，连续的通知会怎样？

<details><summary>核对预测</summary>

计数通知会在任务内核对象中累加，取出时返回累计值。它不保存每个事件的负载。

</details>

**改动题**　把周期改为 500 ms，再读 10 秒期间的累计值。

<details><summary>核对改法</summary>

正常调度下约产生 20 次通知。测量起止相位会影响恰好落在边界的计数。

</details>

## 验证记录

已使用 GNU Arm 14.3.1 与 STM32CubeF4 V1.28.3 编译链接。QEMU `netduinoplus2` 的 Cortex-M4 运行本 ELF 时，两项计数各为 49。天空星的实际节拍与 LED 待验证。
