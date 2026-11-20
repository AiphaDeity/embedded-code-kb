# FreeRTOS 把串口字节交给任务解帧

USART1 中断接收字节，FreeRTOS 任务维护跨多次接收的解帧状态。队列满导致丢字节时，任务用序号缺口识别残帧，从下一字节重新寻找帧头。

## 板型和接线

使用天空星 STM32F407VET6。3.3 V USB 转串口的 TX 接 PA10，RX 接 PA9，两端共地。终端设为 115200、8N1、无流控，关闭本地回显和自动追加换行，使用十六进制收发。

## 沿执行路径读代码

1. `main()` 创建队列和解析任务。`parse_stream()` 在调度开始后挂起第一字节接收，并把 USART1 中断优先级设为 6。该中断可调用 `xQueueSendFromISR()`，需要时通过 `portYIELD_FROM_ISR()` 请求切换。
2. `HAL_UART_RxCpltCallback()` 给字节分配序号，尝试把 `RxItem` 按值放进队列，再立即重挂一个字节。队列满保留已入队的旧字节，丢掉本次新字节，接收序号仍前进。
3. `parse_stream()` 从队列取自己的副本，比较序号。序号跳变表示中间有数据没到，先重置解析器，再处理本字节。UART 硬件错误也留下序号缺口，任务在临界区恢复无 DMA 的接收状态。
4. 公共 `decode_byte()` 读帧头、长度、负载和 XOR 校验。完整并通过校验后，任务保存 `last_payload` 与 `last_length`，再发送两字节应答 `AC 长度`。

帧格式为 `AA 长度 负载 校验`，长度为 1～8。校验初值为长度，逐个 XOR 负载字节。详细状态转换见 [通用 C 字节流解析](../c/stream-frame.md)。中断发送队列元素的规则参见 [FreeRTOS 参考手册](https://www.freertos.org/media/2018/FreeRTOS_Reference_Manual_V10.0.0.pdf)。

## 完整源码

<a href="../../examples/freertos/uart-parser/main.c" download="main.c">下载 main.c</a> · <a href="../../examples/freertos/uart-parser/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a> · <a href="../../examples/c/stream-frame/decoder.h" download="decoder.h">下载共用解帧器</a> · <a href="../../examples/stm32/common/sky-usart1.h" download="sky-usart1.h">下载 USART1 初始化</a>

<<< ../../examples/freertos/uart-parser/main.c

## 构建与观察

```sh
cmake -S examples/freertos/uart-parser -B .work/sky-freertos-uart-parser -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-freertos-uart-parser
```

通过 SWD 加载 `.work/sky-freertos-uart-parser/sky-freertos-uart-parser.elf`，从复位运行。

发送 `AA 02 10 AA B8`，预期收到 `AC 02`，负载中出现的 `AA` 仍作为数据。再发送 `AA 01 02 00 AA 01 7E 7F`，第一帧校验错误，预期只收到 `AC 01`。

## 练习

**预测题**　任务已经读到 `AA 02 33`，下一个接受的队列元素序号跳过了 8。能继续用它拼出原先的帧吗？

<details><summary>核对预测</summary>

不能确认原帧完整。任务重置解帧状态，再把这个字节当成寻找帧头的输入。后续完整帧仍可正常应答。

</details>

**改动题**　把 `pause_parser` 改成 1，发送一段超过 16 字节的数据，再改回 0 并发送一个合法帧。

<details><summary>核对改法</summary>

暂停期间 USART1 继续收字节，队列填满后 `queue_full_drops` 增长。恢复后先取旧数据，收到第一条越过缺口的新元素才增加 `stream_gaps`。合法帧从 `AA` 重新同步。

</details>

## 验证记录

STM32CubeF4 V1.28.3 与 GNU Arm 14.3.1 已编译链接。在 QEMU 的 USART1 上验证了坏长度、坏校验、连续有效帧、部分帧遇到队列丢失时的恢复。固定暂停用例丢掉 8 个新字节，记录 1 处序号缺口，共接受 3 个预期帧，UART 和应答错误数均为 0。天空星的物理串口与真实误码待实板验证。
