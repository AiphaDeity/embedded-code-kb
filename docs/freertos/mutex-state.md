# FreeRTOS 互斥锁保护共享状态

两个任务按不同周期更新一份共享计数。互斥锁保护读、改、写的整体，调试器可以在任务释放锁后观察这份状态。

同一机制也可以保护 [两任务共用的 SPI 事务](spi-mutex.md)。

## 执行顺序

1. 先创建互斥锁，任务拿到锁才能修改 `shared`。
2. 整个读改写在锁内结束，然后释放锁。若等待超时，本轮增加对应的统计值。
3. 调试器可查看 `shared.total`，每个任务成功写入后的累计项分别保存在 `shared.writes[]`。

## 完整源码

<a href="../../examples/freertos/mutex-state/main.c" download="main.c">下载示例 main.c</a>

<<< ../../examples/freertos/mutex-state/main.c

## 构建与观察

使用天空星 STM32F407VET6 的板载 PB2 LED 和 SWD 调试器。工程复用天空星链接脚本，并使用 STM32CubeF4 V1.28.3 固件包内的 FreeRTOS 内核。准备 GNU Arm、CMake、Ninja，将固件包路径换成本机位置：

```sh
cmake -S examples/freertos/mutex-state -B .work/sky-mutex-state -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-mutex-state
```

在实验板上通过 SWD 加载 `.work/sky-mutex-state/sky-mutex-state.elf` 后复位运行。

运行时 `shared.total` 增长。在两个任务都不持有锁时暂停，应看到它与两个 `writes` 之和相等。暂停在临界区中间时，可以单步看完整次更新。

## 练习

**预测题**　能不能只保护 `shared.total++`，把其他记录单独放在锁外？

<details><summary>核对预测</summary>

这里要一致读取整组状态，相关字段应在同一把锁内更新。

</details>

**改动题**　将任务周期改得更接近，观察更新计数和超时统计。

<details><summary>核对改法</summary>

两任务运行次数可以变化，但锁外观察完整更新后的总数和分项仍应一致。

</details>

## 验证记录

已使用 GNU Arm 14.3.1 与 STM32CubeF4 V1.28.3 编译链接。QEMU `netduinoplus2` 的 Cortex-M4 运行本 ELF 时，总写入为 264，两项分别为 165、99，两路超时均为 0。天空星上的调度待验证。
