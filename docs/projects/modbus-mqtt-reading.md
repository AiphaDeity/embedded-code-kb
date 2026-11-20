# 阅读路线 · Modbus–MQTT 网关

这条路线以公开的 Modbus–MQTT 网关源码为阅读对象。解决的问题是“一个从站的寄存器值如何变成上报负载”。[工程仓库](https://github.com/AiphaDeity/modbus-mqtt-gateway/tree/34a5303)

| 顺序 | 从哪里读 | 要确认的状态 |
|---|---|---|
| 1. RS-485 物理收发 | 先读本库的 [RS-485 半双工](../stm32/rs485/half-duplex-echo.md)，再进入 `firmware/app/services/modbus/modbus.c` 的 `StartTaskModbusMaster()` | 请求结束后何时交还总线，接收超时如何返回 |
| 2. 校验应答 | 在同文件找 `validateAnswer()` 的调用点 | 哪条分支接纳本次应答，错误应答有没有进入后续处理 |
| 3. 按点表解释寄存器 | 在 `firmware/app/services/modbus/mb_pointmap.c` 找 `mb_pointmap_parse_slave()` 和 `mb_pointmap_raw_value()` | 寄存器顺序、类型和缩放怎样影响物理量 |
| 4. 组织上报数据 | 在 `firmware/app/services/mqtt/telemetry_task.c` 找 `publish_one_slave()` | 快照是否有效，哪些点进入 JSON，记录序号和内容怎样投入 `storage_m2_write_queue()` |
| 5. 发布已提交记录 | 在 `firmware/app/services/mqtt/mqtt_client.c` 找 `mqtt_replay_pump()` | 从存储取回记录后，如何按种类选择主题并调用 `mqtt_publish_qos1()`，确认后如何按序号交回存储 |

## 带着一次失败读

记录一项点表的站号、寄存器和预期值。先从从站应答确认原始字节，再看解析出来的类型和比例。之后沿同一条记录序号，从采集入队查到存储提交、MQTT 取出和确认。采集日志中的 `queued` 只证明写请求已入队。存储完成提交后，MQTT 才能从存储取到记录。

对单个寄存器值的完整数据路径，可以先跑本库的 [ADC 到 JSON](sensor-to-json.md)，再追网关中对应的数据交接。

## 练习

**预测题**　如果 `validateAnswer()` 报告 CRC 错误，这一轮还能进入寄存器写入分支吗？

<details><summary>核对预测</summary>

错误分支把通信状态置回 `COM_IDLE`，保存错误码并通知发起任务，随后执行 `continue`。寄存器处理在该分支之后，本轮保持此前的值。

</details>

**改动题**　在本库的 [Modbus RTU 示例](../stm32/rs485/modbus-rtu.md)里用同样的有效应答，把其中一个数据位翻转，CRC 保持原值。沿主机自测入口核对结果。

<details><summary>核对改法</summary>

复制有效应答，对 `data[3]` 异或 `0x01`，留下原 CRC，再调用 `read_client_reply()`。预期返回 `REPLY_IGNORED`，原输出值保留。再与网关中通知错误及结束本轮的分支对照。

</details>

## 验证记录

阅读路线和应答错误分支按网关提交 `34a5303` 的代码核对。练习使用本库的独立 RTU 示例，整机路径需要实际项目配置与设备。
