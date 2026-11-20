# 03 · 用 USART1 寄存器收发并回显

从 PA10 接收一个字节，再从 PA9 回传。USART1 配置为 115200、8N1。与 HAL 例子一起读，可对照状态枚举和硬件标志。[HAL USART1 轮询版](polling-echo.md)

## 板型和接线

使用 3.3 V USB 转串口。转接器 TX 接 PA10，RX 接 PA9，GND 与天空星共地。固件从复位默认 HSI 和总线分频 /1 启动。

## 沿执行路径读代码

1. GPIO 引脚切到 AF7，`BRR` 写标称 `0x008B`，`CR1` 打开发送、接收和 USART。
2. `receive_byte()` 先读 `SR`。有数据或出错时紧接着读 `DR`，错误数据丢弃，正确数据交给主循环。
3. 主循环看到 `TXE` 后写 `DR`。`echoed` 代表提交给 USART 的字节数。线路上的最后一位发送完毕要看 `TC`。

## 完整源码

<a href="../../../examples/stm32/uart/register-echo/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/uart/register-echo/main.c

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3，替换固件包路径后在仓库根目录执行。

```sh
cmake -S examples/stm32/uart/register-echo -B .work/sky-uart-register-echo -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-uart-register-echo
```

通过 SWD 加载 `.work/sky-uart-register-echo/sky-uart-register-echo.elf` 后复位运行。串口终端关闭本地回显，发送 `41` 或 `00`（十六进制）并观察原字节回显。静置时 `timeouts` 增长，再发送应恢复回显。

## 练习

**预测题**　半双工切换方向需要等 `TXE` 还是 `TC`？

<details><summary>核对预测</summary>

等 `TC`。`TXE` 只代表发送数据寄存器可以接收下一字节，移位寄存器还可能在发送。

</details>

**改动题**　把接收等待从 100 ms 改为 250 ms，观察静置 1 秒时的计数。

<details><summary>核对改法</summary>

没有字节进来时，`timeouts` 的增长从每秒约十次改为约四次，收到有效字节仍马上回显。

</details>

## 验证记录

目标用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。接线与观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
