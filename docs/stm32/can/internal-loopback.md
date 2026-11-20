# 13b · CAN1 内部回环收发一帧

发送标准数据帧 ID `0x321`，负载 `11 22 33`，轮询 FIFO0 获取回环帧。

有两块板和收发器时，可以接着做 [CAN 双节点请求应答](two-node.md)。

## 板型和接线

只需要天空星核心板和 SWD。示例选静默回环，CAN1 的输出与接收均由内部回环完成，本次实验无需接外部 CAN 收发器。

## 执行顺序

1. 开启 CAN1 时钟，16 MHz APB1 下用 2 分频和 16 TQ 配到标称 500 kbit/s。
2. 用 32 位过滤器接收标准 ID `0x321` 的数据帧，并把它送入 FIFO0。
3. 提交一帧，轮询 FIFO0 至多 100 ms。取出头部和数据后验证 ID、长度和负载。

## 完整源码

<a href="../../../examples/stm32/can/internal-loopback/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/can/internal-loopback/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包路径替换为本机路径，在仓库根目录执行：

```sh
cmake -S examples/stm32/can/internal-loopback -B .work/sky-can-internal-loopback -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-can-internal-loopback
```

用实验板通过 SWD 加载 `.work/sky-can-internal-loopback/sky-can-internal-loopback.elf` 并复位运行。`loopback_ok=1`，`received_id=0x321`，前三个 `received_data` 为 `11 22 33`。`stage` 停在 1～5 时依次检查初始化、过滤器、启动、提交、等接收/取帧。

## 练习

**预测题**　经典 CAN 标准帧的 ID 与数据长度在哪读？

<details><summary>核对预测</summary>

HAL 接收头中的 `StdId` 和 `DLC` 分别表示标准 ID 与数据长度。数据缓冲单独存储。

</details>

**改动题**　只修改发送 ID 为 `0x322`，并保持过滤器不动。

<details><summary>核对改法</summary>

发送可进入邮箱，但 FIFO0 过滤掉该帧，等待接收超时。再把过滤器同步到 `0x322` 即可通过。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译及链接。接线、时序和观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
