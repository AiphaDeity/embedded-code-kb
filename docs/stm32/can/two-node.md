# 13b · 两块天空星用 CAN 请求和应答

请求方发标准帧 `0x321`，应答方用 `0x322` 回传同一 16 位序号。先用[内部回环](internal-loopback.md)确认控制器基本收发，再接真实总线。

## 板型和接线

- 两块天空星 STM32F407VET6，各接一个逻辑侧兼容 3.3 V 的经典 CAN 收发器。
- 每块板的 PB9（CAN1_TX，AF9）接本板收发器 TXD，PB8（CAN1_RX，AF9）接 RXD。按模块说明接正常工作所需的模式脚与电源。
- CANH 接 CANH，CANL 接 CANL，节点共地。总线两个端点各有一只 120 Ω 终端，接线前确认模块是否已带终端。PB8、PB9 是逻辑侧引脚，差分总线接在收发器侧。

## 沿执行路径读代码

1. 两个角色都用正常模式和标称 500 kbit/s。过滤器各接收一个标准 ID，拒绝扩展帧与远程帧。
2. 请求方每轮生成新序号，进邮箱后等 150 ms。应答方从 FIFO0 取到有效帧，使用同一序号回复。
3. 收到 ID、长度和序号都匹配的应答时，`received` 才增长。断开、无应答或错序号会留下计数，下一周期仍发新请求。

## 完整源码

<a href="../../../examples/stm32/can/two-node/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/can/two-node/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。在仓库根目录执行以下命令。

```sh
cmake -S examples/stm32/can/two-node -B .work/sky-can-two-node -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-can-two-node
```

给两块板分别加载结果目录下的 `sky-can-requester.elf` 和 `sky-can-responder.elf`。两板运行时，预计请求方 `submitted` 与有效 `received` 同步增长，`reply_timeouts` 保持 0。逻辑分析仪接收发器逻辑侧可观察两个 ID，若分析差分侧则使用适合 CAN 总线的探头。

停掉应答节点，观察请求方超时和 `last_esr`，再恢复应答节点并核对有效回包重新增加。ESR 中的错误计数和最近错误用于定位链路问题，`submitted` 仅表示 HAL 接收了发送请求。

## 练习

**预测题**　对端发送过滤器不接收的 ID 时，发送方看到总线 ACK 就能确认应用成功了吗？

<details><summary>核对预测</summary>

CAN 控制器可以确认正确接收了总线帧，过滤器仍可把该帧挡在 FIFO 外。应用成功要看协议层回包和匹配的序号。

</details>

**改动题**　只在请求方把 `REPLY_ID` 改成 `0x323`，重新构建和下载这一侧。

<details><summary>核对改法</summary>

应答方的 `0x322` 仍可能出现在总线上。请求方的 FIFO 不接受它，`reply_timeouts` 增长。恢复匹配的 ID 后应恢复通信。

</details>

## 验证记录

请求与应答两个 ELF 均使用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。真实总线、恢复和 ESR 观察值待两块板验证。[STM32F407 数据手册](https://www.st.com/resource/en/datasheet/stm32f407ve.pdf) · [ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
