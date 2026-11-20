# FreeRTOS 用互斥锁保护共享 SPI 总线

两个任务通过同一个 SPI1 读取各自 W25Q128 的 JEDEC ID。两颗芯片各有片选，任务拿到互斥锁才能激活其中一颗。用调试器让任务 A 持锁等待，可以看到任务 B 超时与恢复。

## 板型和接线

使用天空星 STM32F407VET6 裸核心板、两颗 3.3 V W25Q128 模块和 SWD 调试器。用裸核心板可以避开筑基底板上已分配的信号，接线前按手头版本核对引脚。[立创引脚分配](https://wiki.lckfb.com/zh-hans/web-tool/fdb-pinout.html)

| 天空星 | 模块 A | 模块 B |
|---|---|---|
| 3.3 V / GND | VCC / GND | VCC / GND |
| PA5（SCK） | CLK | CLK |
| PA6（MISO） | DO | DO |
| PA7（MOSI） | DI | DI |
| PA4 | CS# | — |
| PB0 | — | CS# |

若模块把 WP# 和 HOLD# 引出来且没有上拉，将它们拉到 3.3 V。未选中的模块需要释放 MISO，才能和另一颗芯片共用 PA6。[Winbond W25Q128JV 数据手册入口](https://winbond.com/hq/support/documentation/?__locale=en&category=%2F.categories%2Fresources%2Fdatasheet%2F&family=%2Fproduct%2Fcode-storage-flash-memory%2Fserial-nor-flash%2Findex.html&line=%2Fproduct%2Fcode-storage-flash-memory%2Findex.html&pno=W25Q128JV)

## 沿执行路径读代码

1. `main()` 先把两个片选置高，再打开 SPI1，创建一把互斥锁。A 与 B 分别保存自己的片选和诊断计数。
2. `device_task()` 最多等 15 个 tick 拿锁。该示例每 tick 为 1 ms。等待超时就记 `lock_timeouts`，本轮不操作 SPI 或片选。
3. 持锁的任务调用 `read_jedec()`。先拉低自己的片选，收发 `9F 00 00 00`，随即拉高片选。HAL 报错时仍由持锁任务尝试中止 SPI，若中止也失败，`bus_stopped` 锁存故障，之后两任务都停止发起事务。
4. 事务完成后才查看应答后三字节。随后任务归还互斥锁，在锁外延时 40 ms。两个任务始终通过同一个句柄访问 SPI1。

互斥锁的 15 ms 对应拿锁时的等待。SPI 收发另传入 20 ms 的 HAL 超时参数。任务优先级不同，FreeRTOS 在高优先级任务等锁时可临时提升持锁者的优先级。[FreeRTOS 互斥锁文档](https://github.freertos.org/Documentation/02-Kernel/02-Kernel-features/02-Queues-mutexes-and-semaphores/04-Mutexes)

## 完整源码

<a href="../../examples/freertos/spi-mutex/main.c" download="main.c">下载示例 main.c</a> · <a href="../../examples/freertos/spi-mutex/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a> · <a href="../../examples/stm32/common/sky-spi1-mode0.h" download="sky-spi1-mode0.h">下载 SPI1 配置</a>

<<< ../../examples/freertos/spi-mutex/main.c

SPI1 初始化和单设备的[裸机 JEDEC ID 示例](../stm32/spi/jedec-id.md)共用。

## 构建和观察

使用 GNU Arm、CMake、Ninja、STM32CubeF4 V1.28.3，把固件包路径替换成自己的路径。

```sh
cmake -S examples/freertos/spi-mutex -B .work/sky-freertos-spi-mutex -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-freertos-spi-mutex
```

通过 SWD 加载 `.work/sky-freertos-spi-mutex/sky-freertos-spi-mutex.elf`，复位后在 Watch 看 `stats[0]` 与 `stats[1]`。接线正确、芯片 ID 为 `EF 40 18` 时，两组 `ids_ok` 都应累加。把 `hold_device_a` 改成 `1` 后继续运行，观察 B 的 `lock_timeouts`。改回 `0`，等当前事务结束，看 B 的 `attempted` 继续增加。

用逻辑分析仪同时夹两个 CS# 和 SCK。两个片选应分别覆盖各自的 32 个时钟，低电平区间应互不重叠。

## 练习

**预测题**　如果 A 拉低片选之后才拿锁，B 持锁发命令时，两颗 Flash 会怎样？

<details><summary>核对预测</summary>

A 的片选已低，B 在锁内也拉低自己的片选，两颗 Flash 会同时接收 B 的时钟和命令，可能同时驱动 MISO。拿锁必须在拉低片选之前，释放锁要在拉高片选和处理传输状态之后。

</details>

**改动题**　保留 A 的 80 ms 等待，把 B 的拿锁时间改成 100 ms，比较前后超时次数。

<details><summary>核对改法</summary>

给 `device_task()` 的等待参数按 `device->id` 选择。B 的等待覆盖正常的 80 ms 注入，拿锁超时通常会消失。比较计数之外，再量一次 B 从开始等锁到拉低片选的延迟。

</details>

## 验证记录

已用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。QEMU Cortex-M4 中两任务先各发起 123 次 SPI 收发，B 的锁超时为 0。开启 A 持锁等待后，B 累计超时 42 次。关掉注入后，B 的收发次数从 185 增到 310，锁超时仍为 42。仿真未连接 W25Q128，两颗芯片的 ID、片选电平和时序待天空星实板验证。
