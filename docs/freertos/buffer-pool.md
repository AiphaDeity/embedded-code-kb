# FreeRTOS 用指针队列借用和归还静态缓冲区

生产者每 25 ms 尝试准备一块 64 字节数据。消费者可以跨任务切换持有这块数据，处理结束再还回去。三块存储从启动一直有效，两条队列只复制 `Buffer *`。

## 沿执行路径读代码

1. `main()` 把三个块地址放入 `free_queue`，再启动调度。调用 `xQueueSend(queue, &buffer, ...)` 时，队列按建立时的 `sizeof(Buffer *)` 从 `&buffer` 复制地址。块内容还在全局池里。
2. `produce()` 借到块以后才开始写。就绪队列收下指针后，生产者不再碰该块。若池空就跳过；若就绪队列已满，就把刚借的块归还。
3. `consume()` 取出指针后独占这个块，延时模拟耗时处理，再逐字节检查内容并归还。还回空闲队列后，下一次生产可以立即复用这块内存。

顺着一个地址跟踪即可看清生命周期。空闲队列 → 生产者 → 就绪队列 → 消费者 → 空闲队列。FreeRTOS 按队列元素大小复制的约定见 [FreeRTOS 参考手册](https://www.freertos.org/media/2018/FreeRTOS_Reference_Manual_V10.0.0.pdf)。

## 完整源码

<a href="../../examples/freertos/buffer-pool/main.c" download="main.c">下载 main.c</a> · <a href="../../examples/freertos/buffer-pool/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../examples/freertos/buffer-pool/main.c

## 构建与观察

用天空星 STM32F407VET6 和 PB2 板载 LED，接供电和 SWD。

```sh
cmake -S examples/freertos/buffer-pool -B .work/sky-freertos-buffer-pool -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-freertos-buffer-pool
arm-none-eabi-nm .work/sky-freertos-buffer-pool/sky-freertos-buffer-pool.elf
```

下载上面的 ELF。让任务自由运行，再暂停查看 `submitted`、`consumed`、`no_free_slot`、`ready_queue_full`。消费者持有一块时，两条待取消息可占满剩余空间，生产者看到池空。消费者归还后再等 80 ms，期间如果就绪队列仍满，生产者可借到块但无法提交。正常情况下 `invalid_payloads` 与 `recycle_errors` 保持 0。

## 练习

**预测题**　如果把 `Buffer packet` 放在生产者循环体内，用 `Buffer *ptr = &packet` 取地址，再调用 `xQueueSend(ready_queue, &ptr, 0)`，消费者能一直正确读取吗？

<details><summary>核对预测</summary>

局部对象只在其声明的作用域有效。队列仍只保存它的地址，消费时局部对象可能已经结束生命周期或已经被下一轮复用。当前示例把实际存储放在全局池，并用两条队列移交访问权限。

</details>

**改动题**　把消费者归还之后的延时改为 10 ms，再比较两种丢弃计数。

<details><summary>核对改法</summary>

只改 `recycle(buffer)` 之后的延时。消费者更快开始取下一块，就绪队列满且还有可借块的窗口会缩短。持有块时的 80 ms 处理延时仍在，池空仍有可能发生。

</details>

## 验证记录

用 STM32CubeF4 V1.28.3 的 FreeRTOS 与 GNU Arm 14.3.1 交叉编译链接。`nm` 确认链接到 `xQueueGenericCreateStatic` 与 `xTaskCreateStatic`，动态分配符号未链接。

在 QEMU `netduinoplus2` 的 Cortex-M4 运行本 ELF 后，读到提交 79、消费 77、池空 168、就绪队列满 247，数据和归还错误均为 0。计数随停止时机变化。天空星现场节拍与 LED 待验证。
