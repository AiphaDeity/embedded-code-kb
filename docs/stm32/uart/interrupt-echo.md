# 03 · USART1 中断收发与回显

发送十六进制 `41`，等回显再发送 `00`、`7E`。接收和回传都通过 USART1 中断推进，主循环负责在它们之间交接字节。

需要暂存多字节时，继续读 [USART1 中断与环形缓冲区](ring-echo.md)。

## 板型和接线

立创天空星 STM32F407VET6，沿用[轮询篇](polling-echo.md)的 3.3 V USB 转串口接线。PA9 TX 接适配器 RX，PA10 RX 接适配器 TX，共地。终端设置 115200 8N1，无流控，关闭本地回显和追加换行。

## 执行顺序

1. 配置 USART1 引脚并使能 NVIC。`USART1_IRQHandler()` 将硬件事件转交 `HAL_UART_IRQHandler()`。
2. `HAL_UART_Receive_IT()` 挂起一次单字节接收。接收完成回调先发布副本，再通知主循环重挂。
3. 主循环短暂屏蔽中断，处理接收恢复和发送缓冲区的所有权。回显开始后便离开临界区。
4. 发送完成回调释放发送缓冲区。遇到接收错误时，主循环中止本轮接收，随后重新挂起。

## 完整源码

<a href="../../../examples/stm32/uart/interrupt-echo/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/uart/interrupt-echo/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包路径替换为本机路径，在仓库根目录执行：

```sh
cmake -S examples/stm32/uart/interrupt-echo -B .work/sky-uart-interrupt-echo -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-uart-interrupt-echo
```

用实验板通过 SWD 加载 `.work/sky-uart-interrupt-echo/sky-uart-interrupt-echo.elf` 并复位运行。空闲时 `loop_count` 持续增长。每次逐字节发送并等回显，`echo_count` 应加一。`dropped_bytes` 记录接收槽已满造成的丢弃。若 `rx_errors` 增长，检查线序、电平和终端波特率。

HAL 的通用 USART 中断处理函数引用了 DMA 取消接口，构建时需要链接 DMA 驱动。本例通过 `HAL_UART_Receive_IT()` 和 `HAL_UART_Transmit_IT()` 启动收发。

## 练习

**预测题**　为什么 TX 回调执行之前不能改写 `tx_byte`？

<details><summary>核对预测</summary>

HAL 持有 `tx_byte` 地址，之后的发送 IRQ 才会从该地址取数据。直到完成回调释放槽位，主循环才能再次赋值。

</details>

**改动题**　在回调处增设计数，比较接收完成次数和发送完成次数，再尝试连续发送一串字节。

<details><summary>核对改法</summary>

逐次等回显时两个计数应同步增长。突发流量可能超出单槽的容量，接收完成次数可以领先，`dropped_bytes` 可能增长。连续流请继续阅读 [UART DMA](../uart-dma-rx-idle.md)。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译及链接。接线、时序和观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
