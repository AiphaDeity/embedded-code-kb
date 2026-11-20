# 阅读路线 · 从天空星 ADC 到 JSON

先用一段已知字节验证主机解析，再接上天空星。实际采样的路径是 **PC0 → ADC1 → 大端采样帧 → USART1 → 主机解帧 → JSON**。

需要将这路 JSON 继续发到 broker 时，沿 [MQTT 上报与 PUBACK](../linux/mqtt-publish.md)读下一段。

## 从哪一步读

| 步骤 | 入口 | 观察什么 |
|---|---|---|
| 1. 采样 | 固件的 `sample_and_send()` | 只有转换完成后才读取原始值，失败增加 `sample_errors` |
| 2. 发送 | 固件的 `HAL_UART_Transmit()` | 每个成功样本送出 5 字节，成功计入 `sent_frames` |
| 3. 解帧 | 主机的 `decode_byte()` | 先判断长度，再接收负载，最后核对校验 |
| 4. 消费 | 主机的 `printf()` | 每份有效 12 位采样单独输出一行 JSON |

## 固件与主机源码

<a href="../../examples/projects/sensor-to-json/firmware/main.c" download="firmware-main.c">下载固件 main.c</a> · <a href="../../examples/projects/sensor-to-json/host/main.c" download="host-main.c">下载主机 main.c</a> · <a href="../../examples/c/stream-frame/decoder.h" download="decoder.h">下载解帧器 decoder.h</a>

<<< ../../examples/projects/sensor-to-json/firmware/main.c

<<< ../../examples/projects/sensor-to-json/host/main.c

## 先在主机验证，再接板

以下命令在 Linux 仓库根目录运行。第一段会送入一份有效帧 `AA 02 03 45 44` 和一份坏校验帧，预期只输出 `{"adc_raw":837}`。

```sh
mkdir -p .work
cc -std=c11 -Wall -Wextra -Werror -Iexamples/c/stream-frame \
  examples/projects/sensor-to-json/host/main.c -o .work/sensor-json
printf '\252\002\003\105\104\252\002\000\001\000' | .work/sensor-json
```

固件使用 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3，在能访问该固件包的电脑上构建。替换示例路径后执行：

```sh
cmake -S examples/projects/sensor-to-json/firmware -B .work/sky-sensor-json -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-sensor-json
```

按 [ADC 篇](../stm32/adc/single-channel.md)接 PC0 和电位器，把 PA9 接 3.3 V USB 转串口的 RX，信号地相连。烧录实验镜像并运行后，在 Linux 设置实际串口设备：

```sh
stty -F /dev/ttyUSB0 raw -echo 115200 cs8 -parenb -cstopb -crtscts -ixon -ixoff clocal
./.work/sensor-json < /dev/ttyUSB0
```

**预测题**　负载中的任一字节恰好等于 `AA` 时，解帧会重新开始吗？

**改动题**　先把第一帧负载低字节改为 `46`，保留旧校验。再修正校验重新运行。

<details><summary>核对结果</summary>

解帧器按长度读取负载，负责判定帧头的是等待帧头状态。修改负载但保留旧校验时没有输出，低字节改为 `46` 后对应校验为 `47`。校验修正后会输出原始值 838。

</details>

## 验证记录

已用 GNU Arm 14.3.1 与 STM32CubeF4 V1.28.3 编译链接固件。Windows 主机以二进制输入核对了正常帧、坏校验、超范围样本以及后续帧的 JSON 输出。Alpine 3.22.8 运行文章中的同一组输入，得到 `{"adc_raw":837}`。PC0 模拟采样到实体串口的整条链路待实板验证。
