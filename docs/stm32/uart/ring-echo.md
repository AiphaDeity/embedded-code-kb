# 03 · USART1 中断接收与环形缓冲区

USART1 中断收到一个字节就交给环形缓冲区。主循环从另一端取出副本并回显。32 个槽留一个判满，有效容量是 31 字节。

## 板型和接线

使用天空星 STM32F407VET6。让 3.3 V USB 转串口的 TX 接 PA10，RX 接 PA9，两端共地。串口设为 115200、8N1、无流控。回显二进制内容时，关闭终端本地回显并使用十六进制发送和显示。

## 沿执行路径读代码

1. `main()` 先调用 `HAL_UART_Receive_IT()` 挂第一字节，再打开 USART1 的 NVIC 通路。
2. `HAL_UART_RxCpltCallback()` 把有效字节交给 `uart_ring_push()`。中断先写入槽，再发布 `head`。若满就丢本次新字节、增加 `ring_full_drops`，然后照常挂下一字节。
3. `uart_ring_pop()` 先复制最早的字节，再推进 `tail` 归还槽。主循环使用自己的副本发送，此时 USART1 RX 仍可中断。
4. 线路错误由 HAL 回调标记，主循环在回调返回后恢复接收，已入队的字节继续处理。发送超时将本字节计入 `tx_failures`，随后继续取新字节。

`head` 只有 ISR 写，`tail` 只有主循环写。原子操作的 release 先发布数据或槽，另一端的 acquire 再使用。同一队列只有一个生产者和一个消费者。需要先读索引绕回的基础写法时，可对照[通用 C 环形缓冲区](../../c/ring-buffer.md)。

## 完整源码

<a href="../../../examples/stm32/uart/ring-echo/main.c" download="main.c">下载 main.c</a> · <a href="../../../examples/stm32/uart/ring-echo/rx_ring.h" download="rx_ring.h">下载数据处理头文件</a> · <a href="../../../examples/stm32/uart/ring-echo/check.c" download="check.c">下载主机检查</a> · <a href="../../../examples/stm32/uart/ring-echo/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../../examples/stm32/uart/ring-echo/main.c

<<< ../../../examples/stm32/uart/ring-echo/rx_ring.h{c}

<a href="../../../examples/stm32/common/sky-usart1.h" download="sky-usart1.h">下载共用 USART1 初始化</a>

## 构建与观察

```sh
cmake -S examples/stm32/uart/ring-echo -B .work/sky-uart-ring-echo -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-uart-ring-echo
```

通过 SWD 加载 `.work/sky-uart-ring-echo/sky-uart-ring-echo.elf`，从复位运行。

发 `41 00 7E FF`，应原样收到四字节。观察 `rx_queued`、`echoed`、`ring_full_drops` 和 `rx_errors`。`ring_full_drops` 是软件缓冲不足，`rx_errors` 来自 USART，例如来不及读取硬件接收寄存器导致的 ORE。

## 练习

**预测题**　主循环暂时忙，环形缓冲区已满，此时收到一个新字节。最早的旧数据会被覆盖吗？

<details><summary>核对预测</summary>

`uart_ring_push()` 先判断下一格是否和 `tail` 相等。满时直接返回，`head` 和旧数据都保留，`ring_full_drops` 加一，接收继续。

</details>

**改动题**　在成功取出字节后加 `HAL_Delay(20U)`，然后连续发送一段多于 31 字节的数据，观察回显和丢弃计数。

<details><summary>核对改法</summary>

把延时放在主循环成功出队之后、发送之前。期间 RX 中断仍可入队。只要后续字节快于主循环释放槽，缓冲区就会填满，已入队字节仍保持原顺序。

</details>

## 验证记录

使用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 交叉编译链接。主机检查了空、满、索引绕回与保序。QEMU 的模拟 USART1 回显 88 个二进制字节，`rx_queued=88`、`echoed=88`，丢弃和错误计数均为 0。真实线路误码和输入速率待实板验证。

```sh
mkdir -p .work
cc -std=c11 -Wall -Wextra -Werror examples/stm32/uart/ring-echo/check.c -o .work/uart-ring-check
.work/uart-ring-check
```
