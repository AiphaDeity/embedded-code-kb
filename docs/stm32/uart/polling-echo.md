# 天空星 UART：USART1 轮询接收与回显

可沿 [CMSIS 寄存器版](register-echo.md) 对照 `SR`、`DR` 与 HAL 的收发状态。

在串口工具中以十六进制发送一个字节 `41`，天空星应原样回显 `41`。等到回显后再发 `00`，收到的也应是 `00`。主循环依次调用 `HAL_UART_Receive()` 和 `HAL_UART_Transmit()`，接收成功才执行发送。

## 板型、接线和串口参数

使用立创·天空星 STM32F407VET6 的 USART1 调试排针，板子单独供电。接一只 **3.3 V 电平**的 USB 转串口适配器。

| 天空星 | USB 转串口 |
|---|---|
| PA9（USART1 TX） | RX |
| PA10（USART1 RX） | TX |
| GND | GND |

串口工具选择适配器的串口号，参数设为 **115200、8 个数据位、无校验、1 个停止位、无流控**。关闭本地回显和自动追加换行，发送和接收显示都选十六进制。USART1 和板载 Type-C 的 USB 数据线走不同引脚。[立创天空星 USART1 引脚资料](https://wiki.lckfb.com/zh-hans/tkx/tkx-stm32f407vxt6/beginner/uart.html)

## 完整示例

<a href="../../../examples/stm32/uart/polling-echo/main.c" download="main.c">下载 UART 轮询示例 main.c</a>

<<< ../../../examples/stm32/uart/polling-echo/main.c

## 顺着 main() 读

1. **初始化。** `HAL_Init()` 建立毫秒时基。`usart1_init()` 分别打开 AHB1 上的 GPIOA 和 APB2 上的 USART1 时钟，再把 PA9 和 PA10 切到 AF7。单看 GPIO 的输出模式无从发送串口数据，复用配置让 USART1 接管引脚。
2. **等到一块数据。** `HAL_UART_Receive()` 的长度参数来自 `sizeof(data)`，默认等 1 字节。HAL 在调用内部轮询 RXNE，收到数据后从数据寄存器读入缓冲区。等待参数为 100 ms，等待期间主循环停在这个调用里。
3. **看返回值再使用数据。** 普通等待超时增加 `rx_timeouts`。如果 HAL 记录了接收错误或返回其他失败，增加 `rx_errors`。STM32CubeF4 V1.28.3 的轮询实现可能在检测到溢出后返回 `HAL_TIMEOUT`，同时设置错误码，因此这里也查 `HAL_UART_GetError()`。接收未成功就回到下一轮，缓冲区里的数据不会发出去。
4. **回显本轮数据。** `HAL_UART_Transmit()` 先等发送数据寄存器有空间，再等最后一帧发送完成。成功增加 `echoed_blocks`，失败增加 `tx_failures`。随后都回到第 2 步等待新数据。

逐字节实验时每发一个字节，等回显再发下一个。发送期间主循环没有进入下一次接收，持续无间隔发送时可能发生接收溢出。接着可以比较 [USART1 中断收发](interrupt-echo.md) 怎样把接收和发送交给中断，以及 [UART DMA 不定长接收](../uart-dma-rx-idle.md) 怎样交接一整块数据。

## 构建并观察

准备 STM32CubeF4 V1.28.3、GNU Arm 编译器、CMake 和 Ninja。将固件包路径改为本机位置，在仓库根目录执行：

```sh
cmake -S examples/stm32/uart/polling-echo -B .work/sky-uart-polling -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-uart-polling
```

通过 SWD 下载 `.work/sky-uart-polling/sky-uart-polling.elf` 到实验板并复位运行。

| 操作 | 预期 |
|---|---|
| 空等约 1 秒 | 串口没有输出，`rx_timeouts` 增长 |
| 发 `41`，收到回显后发 `00` | 依次回显 `41`、`00`，`echoed_blocks` 增加 2 |
| 再空等约 1 秒，然后发 `7E` | 随后仍能回显 `7E` |

若看不到回显，先看 `uart_init_status`，然后看 `rx_timeouts` 是否增长。若只有接收超时增长，先核对适配器 TX 到 PA10、共地和终端参数。若 `echoed_blocks` 已增长，再检查 PA9 到适配器 RX 的连接。

## 练习

**预测题**　已经成功回显过 `41`，下一轮接收一直没有数据。超时后会再次发 `41` 吗？

<details><summary>核对预测</summary>

不会。第 3 步遇到接收失败就执行 `continue`，程序回到第 2 步。第 4 步只处理本轮接收成功的数据。

</details>

**改动题**　将 `ECHO_SIZE` 改成 4。试着在一个接收窗口内一次发送 `41 00 7E FF`，再试只发前三个字节并等待超过一个接收窗口。

<details><summary>核对改法</summary>

一轮内收齐四个字节时，一起回显这四个字节。只收到三个字节时，本轮到期后直接进入下一轮，已写入缓冲区的三字节也不会回显。若完整发送恰好跨过接收窗口，可能被拆到两轮。这里的长度固定为四字节，帧边界由应用自己约定。

</details>

## 验证记录

已按 STM32CubeF4 V1.28.3 HAL 源码核对轮询返回值和超时路径。单字节示例使用 GNU Arm 14.3.1 编译链接成功，四字节练习通过 Arm 编译。用主机模拟 HAL 返回值核对了接收超时、部分收包、溢出错误、发送失败和下一轮继续回显的分支。实体串口的收发和时序待实板验证。
