# 筑基底板 LAN8720A 和裸机 lwIP 回显 UDP

用天空星和筑基底板通过网线接电脑。板端静态地址为 192.168.77.2，监听 UDP 7007。发送 `41 00 7E` 应原样收到三字节，超过 256 字节的负载会丢弃。

## 板型与接线

使用具备 LAN8720A 和 RJ45 的筑基底板。RMII 的 PA1/PA2/PA7、PC1/PC4/PC5、PB11/PB12/PB13 及 PHY 复位 PE12 沿用底板走线。电脑测试接口配置 192.168.77.1/24。示例 MAC 是单设备实验用的本地地址，接入同一网段的多块板应使用各自不同的地址。[筑基板引脚说明](https://wiki.lckfb.com/zh-hans/web-tool/fdb-pinout/introduce.html)

## 沿执行路径读代码

1. 启用 MAC 与 PHY。读取管理总线状态，完成自动协商后配置速率和双工，再向 lwIP 报告链路已就绪。
2. DMA 描述符将已收到的区域借给自定义 pbuf。主循环调用 `ethernet_input`，IPv4 和 UDP 解析后，应用回调核对长度并向原端点回送。
3. 发送路径将 pbuf 链复制到独立缓冲，再交给 HAL。接收 pbuf 的最后一个引用释放后，才把槽归还给 DMA。
4. 所有 lwIP 调用属于主循环。网线断开时同步 link down。DMA 发送报错会锁定发送，等待复位再检查。 [lwIP NO_SYS 模式](https://www.nongnu.org/lwip/2_1_x/group__lwip__opts__nosys.html)

## 完整源码

<a href="../../../examples/stm32/ethernet/udp-echo/main.c" download="main.c">下载 ETH 和主循环</a> · <a href="../../../examples/stm32/ethernet/udp-echo/echo.h" download="echo.h">下载 UDP 回调</a> · <a href="../../../examples/stm32/ethernet/udp-echo/lwipopts.h" download="lwipopts.h">下载 lwIP 配置</a> · <a href="../../../examples/stm32/ethernet/udp-echo/arch/cc.h" download="cc.h">下载移植头</a> · <a href="../../../examples/stm32/ethernet/udp-echo/check.c" download="check.c">下载主机检查</a>

<<< ../../../examples/stm32/ethernet/udp-echo/main.c

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3，替换 SDK 路径。

```sh
cmake -S examples/stm32/ethernet/udp-echo -B .work/sky-ethernet-udp -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-ethernet-udp
```

加载 `.work/sky-ethernet-udp/sky-ethernet-udp.elf` 后，从已配置实验网口的电脑执行下面的单次回显检查。

```python
from socket import AF_INET, SOCK_DGRAM, socket

with socket(AF_INET, SOCK_DGRAM) as endpoint:
    endpoint.bind(("192.168.77.1", 0))
    endpoint.settimeout(2)
    endpoint.sendto(bytes.fromhex("41 00 7E"), ("192.168.77.2", 7007))
    print(endpoint.recv(256).hex(" "))
```

预计打印 `41 00 7e`。可以用 `phy_mode`、`rx_frames`、`echo_stats` 区分链路、驱动和 UDP 层。若准备测试网线恢复，可先在空闲时拔线，核对 link down，插回后再发包。

## 练习

**预测题**　UDP 回调释放 pbuf 后，MAC 会不会再用这块 RX 缓冲发数据？

<details><summary>核对预测</summary>

发送路径已经把链上的数据复制到独立的 TX 内存。RX 槽可以在最后引用释放后归还。

</details>

**改动题**　把最大负载从 256 改为 32，发送 33 字节。

<details><summary>核对改法</summary>

包可以到达 UDP 回调。rejected 增加且没有回包，接收 pbuf 仍会被释放。

</details>

## 验证记录

已用 STM32CubeF4 的实际 lwIP 2.1.2 和 HAL 交叉链接。在主机运行同一协议栈，从模拟网口注入原始 ARP 与 IPv4 UDP 帧，核对正常回复、含零字节负载和超长拒绝。真实 PHY、网线与 DMA 收发待实板核对。[Microchip LAN8720A 数据手册](https://ww1.microchip.com/downloads/en/devicedoc/00002165b.pdf)
