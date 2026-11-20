# 用天空星板载 USB CDC 回显一批字节

电脑打开虚拟串口后写入数据。OUT 回调交接给主循环，主循环提交 IN，确认发送结束才把同一缓冲交回 OUT。

## 板型与接线

使用 STM32F407VET6 天空星带数据的 Type-C 接口及数据线。板上 PA11/PA12 是 USB D−/D+。示例用 HSE 8 MHz 配 PLLQ 48 MHz 和内核 84 MHz。

## 沿执行路径读代码

1. HAL PCD 负责底层端点，设备栈处理枚举，CDC 接口负责数据交接。
2. 接到一个最长 64 字节的包后，当前缓冲暂不挂回 OUT。主循环提交原数据，后续 USB OUT 由端点背压等待。
3. IN 完成后才重挂 OUT。64 字节整数包的结束处理由 CDC 类负责，复位或重新配置会回到初始接收状态。

## 完整源码

<a href="../../../examples/stm32/usb/cdc-echo/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/usb/cdc-echo/main.c

<a href="../../../examples/stm32/usb/cdc-echo/usb_port.c" download="usb_port.c">下载 HAL PCD 适配</a> · <a href="../../../examples/stm32/usb/cdc-echo/descriptors.c" download="descriptors.c">下载设备描述符</a> · <a href="../../../examples/stm32/usb/cdc-echo/usbd_conf.h" download="usbd_conf.h">下载设备栈配置</a>

构建默认使用非唯一测试编号 1209:0001，用于本地原型。分发设备时配置分配给你的 VID/PID，CMake 选项为 SKY_USB_VID 和 SKY_USB_PID。

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包目录换成本机路径后，在仓库根目录运行以下命令。

```sh
cmake -S examples/stm32/usb/cdc-echo -B .work/sky-usb-cdc -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-usb-cdc
```

下载 .work/sky-usb-cdc/sky-usb-cdc.elf 后，用 Type-C 重新连接，查看系统是否新增 COM 端口或 ttyACM。关闭终端本地回显，测试 41 00 7E、64 字节和超过 64 字节的连续数据，核对原样返回及 rx_packets、tx_packets。

## 练习

**预测题**　IN 还没完成时，可以把同一缓冲挂回 OUT 吗？

<details><summary>核对预测</summary>

那会允许新数据覆盖尚未发送完的内容。示例等到发送完成再重挂。

</details>

**改动题**　回显中只把小写 ASCII 改为大写。

<details><summary>核对改法</summary>

在 TX_PENDING 分支中提交 IN 前改写本轮有效字节。只改 0x61～0x7A，验证其余二进制字节保持。

</details>

## 验证记录

已编译链接 STM32CubeF4 V1.28.3 的 USB Device、CDC 和 HAL PCD。实物枚举、64 字节边界与断线重连待板上验证。 [天空星核心板介绍](https://wiki.lckfb.com/zh-hans/fdb/micropython/introduction.html) · [pid.codes 测试编号](https://pid.codes/1209/0001/)
