# CAN 中断收帧，主循环批量写入 SD 卡

天空星在一条运行中的 500 kbit/s CAN 总线上静默监听，将标准帧、扩展帧和远程帧按顺序记录到 SD 卡上的 `CANLOG.CSV`。

## 板型与接线

使用配有外接 3.3 V CAN 收发器和原生 SD 卡座的 STM32F407VET6 天空星。PB8 接收发器 RXD，PB9 接 TXD，并共地。SDIO 的 PC8、PC12、PD2 分别接 DAT0、CLK、CMD，准备一张已有 FAT16/FAT32 的实验卡。[SDIO 接线与供电](../stm32/sdio/fatfs-log.md)

用前面的[双节点 CAN 示例](../stm32/can/two-node.md)产生流量。记录板是第三个节点，静默模式下不会给发送端 ACK，现有两节点正常完成总线确认。总线的两端保持 120 Ω 终端。

## 沿执行路径读代码

1. CAN 接收 IRQ 从 FIFO0 读出帧和毫秒时间，写入 15 条有效容量的环形队列。写完一条才公布队首，下次主循环能够得到完整副本。队列满时保留旧帧，丢掉新到帧并增加 `queue_drops`。
2. 主循环每 250 ms 取最多 8 条，在 RAM 中格式化为一个批次。每行依次为毫秒、标准或扩展、十六进制 ID、数据或远程、DLC、数据。
3. 整批交给 FatFs。只有关闭并同步成功才增加 `lines_written`。写失败后停在阶段 3，保留 FatFs 和 SDIO 的状态供排查，卡上可能已有本批次的部分内容。

## 完整源码

<a href="../../examples/projects/can-to-sd/main.c" download="main.c">下载示例 main.c</a> · <a href="../../examples/projects/can-to-sd/can_log.h" download="can_log.h">下载队列与格式化</a> · <a href="../../examples/projects/can-to-sd/check.c" download="check.c">下载主机检查</a> · <a href="../../examples/stm32/sdio/fatfs-log/sdio_disk.c" download="sdio_disk.c">下载共用 SDIO diskio</a>

<<< ../../examples/projects/can-to-sd/main.c

<<< ../../examples/projects/can-to-sd/can_log.h{c}

## 编译与观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3，替换 SDK 路径。

```sh
cmake -S examples/projects/can-to-sd -B .work/sky-can-to-sd -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-can-to-sd
```

加载 `.work/sky-can-to-sd/sky-can-to-sd.elf`，产生带 ID 0x321 和 0x322 的请求应答后停掉流量，等待 `lines_written` 稳定，断电后取卡查看。标准数据帧的一行示例为 `123,S,00000321,D,2,00AB`。S/E 区分帧格式，D/R 区分数据与远程。

## 练习

**预测题**　SD 卡写入变慢，持续进来的 CAN 帧会怎样？

<details><summary>核对预测</summary>

IRQ 仍可入队。队列填满 15 条后开始丢新帧，queue_drops 增加。恢复到能够消费的速率后，新帧可以再次进入队列。

</details>

**改动题**　只保留 ID 0x321 的标准数据帧。

<details><summary>核对改法</summary>

在接收回调入队前核对 IDE、RTR 和 StdId。重新运行双节点请求应答，文件中应保留 0x321。

</details>

## 验证记录

已交叉编译并链接。GCC 主机检查验证队列满、顺序跨回绕、标准数据帧和扩展远程帧输出，以及非法长度。共用 FatFs 路径的原内存盘测试已通过。实际总线与 SD 卡联合运行待实板。
