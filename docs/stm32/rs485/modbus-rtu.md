# 13a · RS-485 上请求和应答 Modbus RTU 寄存器

请求端发送 01 03 00 00 00 02 C4 0B，读取单元 1 从零偏移起的两个保持寄存器。应答端返回 0x1234 和 0xABCD，或针对合法但不支持的请求回复异常码。

## 板型与接线

两块天空星各接 3.3 V MAX3485。PA9 接 DI，PA10 接 RO，PA8 同时接 DE 和 /RE 并加下拉。两侧差分端、参考地、终端和空闲偏置按模块标记连接。实验端口为 9600 8E1。

## 沿执行路径读代码

1. TIM2 以 1 μs 计数。采集 RXNE 的时刻反映字符已收到，两次 RXNE 的间隔包含后一个字符。代码在 1.5 和 3.5 字符门限上各加一个字符时间，用 2865 和 5157 μs 作实验门限。
2. 帧内异常、过长、错误 CRC 和站号不匹配都丢弃。应答端收到 03 后验证起始和数量，再按高字节在前写寄存器值、低 CRC 字节在前写校验。
3. 两端发送时拉高 DE，HAL 发送完成后释放。请求端一次仅等一个应答，150 ms 到期回到下一轮。

## 完整源码

<a href="../../../examples/stm32/rs485/modbus-rtu/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/rs485/modbus-rtu/main.c

<a href="../../../examples/stm32/rs485/modbus-rtu/rtu.h" download="rtu.h">下载帧处理</a> · <a href="../../../examples/stm32/rs485/modbus-rtu/check.c" download="check.c">下载主机检查</a> · <a href="../../../examples/c/crc16-modbus/crc16.h" download="crc16.h">下载 CRC-16</a>

<<< ../../../examples/stm32/rs485/modbus-rtu/rtu.h{c}

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包目录换成本机路径后，在仓库根目录运行以下命令。

```sh
cmake -S examples/stm32/rs485/modbus-rtu -B .work/sky-modbus-client -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-modbus-client
```

在 .work/sky-modbus-client 取得 sky-modbus-client.elf 和 sky-modbus-server.elf，分别下载到两块板。客户端 value0、value1 预计为 0x1234、0xABCD。断开服务端可观察 response_timeouts，恢复接线应继续收包。串口分析应显示最后停止位之后 DE 才回到接收。

## 练习

**预测题**　目标站号或 CRC 错误会收到 Modbus 异常码吗？

<details><summary>核对预测</summary>

这两种错误直接丢弃。站号与 CRC 均正确且功能或参数无效时，才有协议异常应答。

</details>

**改动题**　在请求端把起始偏移改成 1，数量保持 2。

<details><summary>核对改法</summary>

请求跨过了服务端已有的两个寄存器。预期 last_exception 记下 02，上一组正常值保留。

</details>

## 验证记录

Windows 主机重放已验证准确请求帧、微秒计数回绕、长度溢出、普通应答、异常 02/03 与坏 CRC。两份固件已交叉链接，真实总线定时和收发待验收。 [Modbus RTU 串行规范](https://www.modbus.org/file/secure/modbusoverserial.pdf)

运行独立主机检查。

```powershell
New-Item -ItemType Directory -Force .work | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/stm32/rs485/modbus-rtu/check.c -o .work/modbus-rtu-check.exe
& ./.work/modbus-rtu-check.exe
```
