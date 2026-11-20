# 天空星用 UART DMA 连续接收两批不定长数据

用串口工具以十六进制发送 `11 22 33`，等回显后再发 `AA BB`。目标是两批都原样返回。先读裸机的 `main()`，再跟着 `HAL_UARTEx_RxEventCallback()` 找第二次接收在哪里启动。

第一次读 USART1，可以先做[轮询接收与回显](uart/polling-echo.md)，再读[中断收发](uart/interrupt-echo.md)，最后沿用相同的引脚和终端设置观察 DMA 怎样一次交接一块数据。

## 板型与接线

示例以**立创·天空星 STM32F407VET6**为参考。USART1 走核心板调试排针。使用 3.3 V 电平 USB 转串口适配器，板子单独供电。适配器 TX 接 PA10（板端 RX），适配器 RX 接 PA9（板端 TX），两端共地。终端选适配器串口，115200、8 位、无校验、1 停止位，关闭本地回显和自动追加换行。核心板 Type-C 的 USB 数据引脚与 USART1 调试排针各自独立。[立创天空星资料](https://wiki.lckfb.com/zh-hans/tkx/tkx-stm32f407vxt6/) · [官方串口 DMA 教程](https://wiki.lckfb.com/zh-hans/tkx/tkx-stm32f407vxt6/beginner/dma.html)

## 构建与运行

仓库的 `examples/stm32/sky-uart/` 含板级代码和 CMake 入口，`examples/stm32/common/` 提供天空星链接脚本，`examples/freertos/sky-uart-queue/` 含队列应用。准备 GNU Arm 编译器、CMake、Ninja 和 STM32CubeF4 V1.28.3 完整固件包。在仓库根目录执行以下命令，将固件包路径换为本机实际位置。

```sh
cmake -S examples/stm32/sky-uart -B .work/sky -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky
```

两个可单独启动的镜像位于 `.work/sky/`。本篇用 `sky-bare.elf`，队列篇用 `sky-queue.elf`。镜像从片内 Flash 起始地址启动，按 512 KiB Flash 链接。实板复现时先核对芯片丝印，在实验板上用 SWD 调试器下载所选 ELF，复位后打开串口。

## 从主循环读起

<a href="../../examples/stm32/sky-uart/main.c" download="main.c">下载裸机入口 main.c</a> · <a href="../../examples/stm32/sky-uart/board.c" download="board.c">下载串口与 DMA 实现 board.c</a>

<<< ../../examples/stm32/sky-uart/main.c

1. `board_init()` 使用内部 HSI 时钟，配置 PA9 / PA10 为 AF7，打开 USART1 与 DMA2 Stream5 Channel4。DMA 普通模式的接收缓冲区容量为 32 字节。
2. `board_start_rx()` 启动一轮接收。短包到达后线路空闲，USART1 中断进入 HAL，再到接收回调。填满 32 字节时，DMA 完成中断也能结束这一轮。半传输事件直接返回，避免重复消费前半段。
3. `app_rx_from_isr()` 在重启 DMA 前把有效长度和数据按值复制到 `pending`。主循环在短临界区取本地副本，然后释放单槽并回传。单槽尚未取走时，新块丢弃，`dropped_blocks` 加一。
4. 回调末尾再次调用 `board_start_rx()`，下一批从下标 0 开始。普通模式在两次启动之间有时间间隙；本例采用逐批应答。空闲事件表示线路出现间隔，完整协议帧还需要上层解析。

<<< ../../examples/stm32/sky-uart/board.c

## 串口收一次就停，从哪查

先确认第二批到达后接收回调有没有执行。若没有，观察 `fault_code` 是否为 4（启动接收失败），核对串口和 DMA 的两个中断入口。若有回调却缺回显，观察 `dropped_blocks` 和回传错误码 5。断点会改变收发时序，连续接收实验优先看回显和计数器。

**预测题**　只发 16 字节，随后保持空闲，会不会因为半传输和空闲各回传一次？

<details><summary>核对预测</summary>

半传输回调直接返回，空闲结束这一轮时才交给应用，预期只回传一次。

</details>

**改动题**　在主循环成功回传后增加块计数和字节计数，分别发三字节和两字节，每次等回显再发下一批。

<details><summary>核对结果</summary>

计数初值为零时，预期得到 2 块、5 字节，丢弃计数为 0。

</details>

## 验证记录

使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译并链接裸机 ELF。两批回显、缓冲区满和异常恢复待实板验证。接下来可对照[FreeRTOS 队列交接](../freertos/queue-message.md)。
