# 在天空星上用 FreeRTOS 队列交接串口数据

使用[裸机篇的接线和构建方式](../stm32/uart-dma-rx-idle.md)，选择 `sky-queue.elf`。用同一批串口数据观察两个问题：DMA 已经复用缓冲区时，任务读到谁的数据？当任务落后，哪一条消息会被丢弃？

需要在任务里处理跨多次接收的协议数据，可看[串口任务解帧和缺口恢复](uart-parser.md)。

## 从队列元素的大小读起

<a href="../../examples/freertos/sky-uart-queue/main.c" download="main.c">下载任务入口 main.c</a> · <a href="../../examples/freertos/sky-uart-queue/FreeRTOSConfig.h" download="FreeRTOSConfig.h">下载内核配置</a>

<<< ../../examples/freertos/sky-uart-queue/main.c

1. `main()` 创建深度为 4 的队列，元素大小为 `sizeof(RxBlock)`，每项含有效长度和 32 字节数组。先创建队列和任务，启动调度器后才由任务开启 DMA。
2. 在回调中，`xQueueSendFromISR()` 将整块内容复制进队列。DMA 可以重新使用原始缓冲区。队列满时保留已入队的块，并对丢弃的新块计数。
3. `xQueueReceive()` 将一条消息复制到任务的 `local` 变量，然后按有效长度回传。无消息时任务阻塞；中断调用 `portYIELD_FROM_ISR` 请求必要的任务切换。
4. USART1 和 DMA 的抢占优先级都设为 6。工程使用四个抢占优先级位，允许调用内核 ISR 接口的边界配置为 5，数值 6 符合该约束。内核检查保留在 `configASSERT` 中。[FreeRTOS Cortex-M 中断约定](https://ecos.freertos.org/Documentation/02-Kernel/03-Supported-devices/04-Demos/ARM-Cortex/RTOS-Cortex-M3-M4)

## 做两次观察

- **正常速度**。令 `consumer_delay_ms` 为 0，以十六进制分别发送 `11 22 33`、`AA BB`，等回显再发送下一批。预期每批回显与该批发送内容一致，长度依次为 3 和 2。
- **制造积压**。在调试器中将 `consumer_delay_ms` 改为 500，让任务每次回传后等 500 ms。用串口工具每隔约 30 ms 发送一批短数据，连续十批。观察 `dropped_blocks` 是否增长。停止发送，改回 0，待积压消化后再发一批检查恢复。实际丢弃数与发送和调度时序有关。

**预测题**　新数据覆盖 DMA 缓冲区，已入队的上一条消息会改变吗？

<details><summary>核对预测</summary>

队列内已有一份完整的结构体副本。若改为只传指针，就需要明确谁持有缓冲区、消费者何时归还，否则 DMA 可能覆盖任务正在读取的数据。

</details>

**改动题**　把队列深度改为 1，重复积压实验。记录第一条回显、丢弃计数和恢复后最后一条数据。

<details><summary>核对观察点</summary>

任务延时时队列只能容纳一个待取块，后续到达的块走队列满分支。任务恢复后先处理已入队的块。

</details>

## 验证记录

使用 STM32CubeF4 V1.28.3 自带的 FreeRTOS V10.3.1 和 GNU Arm 14.3.1 完成编译与链接。回显、积压与恢复待天空星实板复测。
