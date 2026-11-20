# 静态创建 FreeRTOS 队列和任务，观察剩余栈

两个任务用一个两槽队列交接采样消息。控制块、消息区、任务栈和 Idle 任务栈都由应用提供。生产周期 50 ms，消费每次等 125 ms，用运行计数观察队列被填满。

需要传较大的块并由接收任务归还时，可以继续看[指针队列和静态缓冲池](buffer-pool.md)。

## 沿执行路径读代码

1. 用全局数组提供队列消息区和两个任务的栈。调度器启动阶段通过 `vApplicationGetIdleTaskMemory()` 获取 Idle 的控制块与栈。
2. 生产者按值入队。队列满时丢弃本次新消息；消费者继续按入队顺序取出旧消息。
3. 各任务调用 `uxTaskGetStackHighWaterMark(NULL)`，记录自创建起最小的剩余栈字数。历史栈余量只覆盖已经走过的代码路径。

## 接线和源码

使用天空星 STM32F407VET6 和 PB2 板载 LED，连接供电和 SWD，不需要外接模块。

<a href="../../examples/freertos/static-queue/main.c" download="main.c">下载示例 main.c</a>

<<< ../../examples/freertos/static-queue/main.c

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3，从仓库根目录执行。

```sh
cmake -S examples/freertos/static-queue -B .work/sky-freertos-static-queue -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-freertos-static-queue
```

通过 SWD 加载 `.work/sky-freertos-static-queue/sky-freertos-static-queue.elf` 后复位运行。运行后 `max_queued` 预计达到 2，`dropped` 随后增加，`sent` 和 `received` 仍继续增加。看两个 `*_min_free_words`，一个 Cortex-M4 栈字为 4 字节。

## 练习

**预测题**　生产任务本轮结束后，队列中的局部 `packet` 还有效吗？

<details><summary>核对预测</summary>

入队时 FreeRTOS 已把 `sizeof(Packet)` 字节复制到队列存储区。消费任务读到的是该副本。

</details>

**改动题**　把消费者的延时缩短到 25 ms，比较丢包数。

<details><summary>核对改法</summary>

消费等待缩短到生产周期的一半，正常调度下队列不该持续堆积。复位重跑后观察 `dropped` 和 `max_queued`。

</details>

## 验证记录

已使用 GNU Arm 14.3.1 与 STM32CubeF4 V1.28.3 编译链接。QEMU `netduinoplus2` 的 Cortex-M4 上，发送 101、接收 99、因队列满丢弃 147，队列峰值为 2。两项最小剩余栈分别为 228 和 231 个字。天空星上需重新测量栈余量及调度。
