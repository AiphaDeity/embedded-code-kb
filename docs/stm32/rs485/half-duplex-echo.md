# 13a · RS-485 半双工两节点回显

终端使用 9600 8N1，每次发送一个字节并等回显。可用逻辑分析仪同步观察 PA8 的方向控制和 PA9 的发送。

## 板型和接线

天空星加外置 3.3 V MAX3485 收发器，PA9 接 DI、PA10 接 RO，PA8 同时接 DE 和 /RE，在该控制线上加下拉，使上电先处于接收。另一节点为可自动切换方向的 USB-RS485 适配器，按双方手册对应互接差分端与信号参考地，终端与空闲偏置按模块配置。

[Analog Devices MAX3485 资料](https://www.analog.com/en/products/max3485.html)

## 执行顺序

1. 打开 GPIOA 和 USART1。PA8 预置低电平后转输出，驱动器保持接收。
2. 轮询收到一个字节后等待 3 ms，让本实验中的自动方向适配器有时间释放总线。
3. 拉高 DE 后调用 HAL 发送。成功路径会等 TC，也就是停止位送出后才返回。
4. 发送调用结束就撤销驱动，回到下一轮接收。错误路径也释放总线，并累加失败计数。

## 完整源码

<a href="../../../examples/stm32/rs485/half-duplex-echo/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/rs485/half-duplex-echo/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包路径替换为本机路径，在仓库根目录执行：

```sh
cmake -S examples/stm32/rs485/half-duplex-echo -B .work/sky-rs485-half-duplex-echo -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-rs485-half-duplex-echo
```

用实验板通过 SWD 加载 `.work/sky-rs485-half-duplex-echo/sky-rs485-half-duplex-echo.elf` 并复位运行。每发一个字节预期收到同一字节。试用示波器确认 PA8 高覆盖 PA9 发送的全部停止位。若全无回显，检查 A/B 定义、参考地以及适配器释放总线的时间。3 ms 仅为实验间隔，依据对端实际时序调整。

## 练习

**预测题**　为什么不能看到 TXE 就撤销 DE？

<details><summary>核对预测</summary>

TXE 表示发送数据寄存器可写，移位寄存器仍可能在发送。TC 表示最后一帧已发送完成。

</details>

**改动题**　把波特率改为 19200，同时修改终端设置，再观察 DE 的高电平持续时间。

<details><summary>核对改法</summary>

单个 8N1 字符的线路时间约减半，方向切换仍须等发送完成。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译及链接。接线、时序和观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
