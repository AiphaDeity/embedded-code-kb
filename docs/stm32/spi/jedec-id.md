# 08 · SPI 读取 W25Q128 JEDEC ID

拉低片选后发 `9F 00 00 00`，同时收四个字节。预期后三字节为 `EF 40 18`，可在 Watch 中查看。

## 板型和接线

以裸天空星核心板和外接 3.3 V W25Q128 模块为例。SPI1 使用 PA5 → CLK、PA6 ← DO、PA7 → DI，PA4 → CS，电源 3.3 V，共地。WP#、HOLD# 均拉高。使用前核对核心板和扩展板版本的引脚占用，确保这组引脚可用。

## 执行顺序

1. PA4 作为片选输出，先预置高电平，再把引脚设为输出。
2. `sky_spi1_mode0_init()` 配置 SCK、MISO、MOSI 为 AF5，SPI1 主模式使用 8 位、Mode 0、软件 NSS，SCK 约 1 MHz。
3. 一次收发发送完整事务。命令字节期间收到的第一字节没有用于 ID，数据阶段继续发占位字节产生时钟。
4. 无论成功或失败均释放片选。只有成功才更新三个 ID 字节。

## 完整源码

<a href="../../../examples/stm32/spi/jedec-id/main.c" download="main.c">下载示例 main.c</a> · <a href="../../../examples/stm32/common/sky-spi1-mode0.h" download="sky-spi1-mode0.h">下载 SPI1 配置</a>

<<< ../../../examples/stm32/spi/jedec-id/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包路径替换为本机路径，在仓库根目录执行：

```sh
cmake -S examples/stm32/spi/jedec-id -B .work/sky-spi-jedec-id -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-spi-jedec-id
```

用实验板通过 SWD 加载 `.work/sky-spi-jedec-id/sky-spi-jedec-id.elf` 并复位运行。典型 W25Q128 模块的 `jedec` 是 `{0xEF,0x40,0x18}`，具体型号按芯片数据手册确认。如果读到 `FF` 重复，先检查片选和 MISO 是否连通。

## 练习

**预测题**　读取阶段为什么还要发送三个占位字节？

<details><summary>核对预测</summary>

主机发送时才产生 SCK。读取三个字节需要主机提供 24 个 SCK 周期。

</details>

**改动题**　在 `sky-spi1-mode0.h` 中把 SPI 预分频从 16 改为 32，用逻辑分析仪对照同一条命令的时钟和片选。复用该头文件的 [FreeRTOS 共享 SPI 示例](../../freertos/spi-mutex.md)也会采用这份配置。

<details><summary>核对改法</summary>

SCK 频率从标称 1 MHz 变成约 500 kHz，读回的 JEDEC 字节仍应一致。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译及链接。接线、时序和观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
