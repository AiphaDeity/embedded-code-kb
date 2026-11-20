# 天空星通过 USART1 查询 ESP-AT 的 Wi-Fi 状态

用 USART1 向外部 ESP32 发 `AT` 和 `AT+CWSTATE?`。程序等完整的 `OK` 终止行，再把查询结果交给调试器。

## 模块和接线

使用已烧录 ESP-AT、支持 `AT+CWSTATE?` 的 ESP32-WROOM-32 开发板。ESP32 按开发板规格供电，两板共地，信号为 3.3 V。以下按官方默认 AT UART1 引脚，实际以模块和 AT 固件配置为准。

| 天空星 | ESP32-WROOM-32 AT 串口 |
|---|---|
| PA9（USART1 TX） | GPIO16（RX） |
| PA10（USART1 RX） | GPIO17（TX） |
| GND | GND |

两端为 115200、8N1、无流控。用独占的 AT 串口，避免两个 TX 同时驱动模块 RX。其他型号先核对 [ESP-AT 官方接线](https://docs.espressif.com/projects/esp-at/en/latest/esp32/Get_Started/Hardware_connection.html)。

## 沿执行路径读代码

1. `main()` 配好 UART，给模块留出启动时间，用 `AT` 握手。握手失败就保留结果并停住。
2. `command()` 每次发一条带 CRLF 的命令。所有接收字节共用 500 ms，到行尾由 `at_feed()` 识别回显、主动通知、中间结果和终止行。
3. `+CWSTATE:` 提供临时状态，`OK` 才完成本轮。成功后主循环保存 `wifi_state`，等待一秒再查。超时、`ERROR`、`busy` 或缺少状态行时保留错误和旧状态，检查后复位两端再试。

## 完整源码

<a href="../../../examples/stm32/wireless/esp-at-status/main.c" download="main.c">下载 main.c</a> · <a href="../../../examples/stm32/wireless/esp-at-status/at_reply.h" download="at_reply.h">下载行解析器</a> · <a href="../../../examples/stm32/wireless/esp-at-status/check.c" download="check.c">下载主机检查</a> · <a href="../../../examples/stm32/wireless/esp-at-status/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../../examples/stm32/wireless/esp-at-status/main.c

<<< ../../../examples/stm32/wireless/esp-at-status/at_reply.h{c}

## 编译与观察

```sh
cmake -S examples/stm32/wireless/esp-at-status -B .work/sky-esp-at-status -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-esp-at-status
```

通过 SWD 加载 `.work/sky-esp-at-status/sky-esp-at-status.elf`，看 `result`、`attempt` 和 `wifi_state`。状态 `0` 表示尚未启动连接，`2` 表示已连入并取得 IPv4。此例查询模块当前状态。也可用逻辑分析仪按 115200 解码 PA9、PA10，看命令末尾的 CRLF 和模块的终止行。

## 练习

**预测题**　模块先回 `WIFI CONNECTED`，再回 `+CWSTATE:2,"lab"`，随后回 `OK`。调试器中的 `wifi_state` 何时改变？

<details><summary>核对预测</summary>

状态行先更新本轮的局部解析器。`OK` 在期限内完整到达后，主循环才复制到 `wifi_state`。超时会保留旧值。

</details>

**改动题**　将主机检查里的有效状态行从 `2` 改为 `4`，核对何时得到 `AT_MORE` 与 `AT_OK`。

<details><summary>核对改法</summary>

将 `ATE:2` 改为 `ATE:4`，相应状态断言改成 4。有效中间行返回 `AT_MORE`，终止行返回 `AT_OK`。

</details>

## 验证记录

目标 ELF 已交叉编译链接。主机验证拆分回复、主动通知、确认、错误、忙和超长行。实际模块接线和状态变化待实板。

```sh
mkdir -p .work
cc -std=c11 -Wall -Wextra -Werror examples/stm32/wireless/esp-at-status/check.c -o .work/esp-at-check
.work/esp-at-check
```

回复和状态定义见 [Espressif 基础 AT 命令](https://docs.espressif.com/projects/esp-at/en/latest/esp32/AT_Command_Set/Basic_AT_Commands.html)及 [AT+CWSTATE](https://docs.espressif.com/projects/esp-at/en/latest/esp32/AT_Command_Set/Wi-Fi_AT_Commands.html#at-cwstate-query-the-wi-fi-state-and-wi-fi-information)。
