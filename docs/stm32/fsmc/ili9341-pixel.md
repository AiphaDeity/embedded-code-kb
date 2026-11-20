# 10 · FSMC 给 ILI9341 并口屏写入像素

向两个外存映射地址写命令和数据，在屏幕左上区域填一个 32×32 色块。调成 1×1 可观察一次像素写入的总线波形。

若要通过同一控制器访问多地址存储器，可对照 [FSMC 复用地址并读写外部 SRAM](external-sram.md)。

## 板型和接线

以裸天空星和已配置为 **16 位 8080 MCU 接口**的 ILI9341 模块为例，选择带完整并口、3.3 V 逻辑且按模块额定值独立供电的屏。数据连接如下，模块 D0～D15 指其 16 位并口的逻辑位，裸 IC 的 IM 模式与实际引脚映射须看手册。

| 信号 | STM32 |
|---|---|
| D0～D3 | PD14、PD15、PD0、PD1 |
| D4～D12 | PE7～PE15，依次对应 |
| D13～D15 | PD8、PD9、PD10 |
| CS#、WR#、RD# | PD7、PD5、PD4 |
| D/C、RESET# | PD11（A16）、PC6 |

共地，模块背光按其电源规范连接。PD/PE 引脚须在所用版本排针上实际可用。

[ILI9341 厂商规格书](https://robu-prod-media.s3.ap-south-1.amazonaws.com/uploads/2018/02/ILI9341_v1.11.pdf)

## 执行顺序

1. 把数据、读写选通和 A16 配成 FSMC AF12，选择 Bank1/NE1 的异步 16 位通道。
2. 在 16 位总线上，CPU 字节地址 bit17 接到外部 A16。两个目标地址之间相差 `0x20000`。
3. 硬复位后退出休眠，选择 RGB565，设定列与行窗口。`0x2C` 后的每次半字写入就是一个像素。
4. 软件每秒在红色和绿色间切换，便于确认窗口与像素传输。

## 完整源码

<a href="../../../examples/stm32/fsmc/ili9341-pixel/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/fsmc/ili9341-pixel/main.c

本例的 FSMC 初始化与矩形刷新也用于[触摸标记](ili9341-touch.md)。<a href="../../../examples/stm32/fsmc/common/ili9341.h" download="ili9341.h">下载共用显示驱动</a>

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包路径替换为本机路径，在仓库根目录执行：

```sh
cmake -S examples/stm32/fsmc/ili9341-pixel -B .work/sky-fsmc-ili9341-pixel -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-fsmc-ili9341-pixel
```

用实验板通过 SWD 加载 `.work/sky-fsmc-ili9341-pixel/sky-fsmc-ili9341-pixel.elf` 并复位运行。正常时 32×32 区域在两种颜色间切换。可用逻辑分析仪看 CS#、WR#、D/C，命令写入时 D/C 低，像素阶段高。若模块需要额外电源或面板初始化，先按其给出的序列适配。

## 练习

**预测题**　为什么不是 `0x60010000`？

<details><summary>核对预测</summary>

FSMC 选择 16 位数据宽度时，外部 A0 对应 CPU 字节地址 bit1。A16 因此对应 bit17。

</details>

**改动题**　把 `SIDE` 改为 1，触发一次写窗口和写像素。

<details><summary>核对改法</summary>

目标窗口两端坐标相同，RAMWR 后仅写一个半字，肉眼不易辨认时优先看 WR# 脉冲。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译及链接。接线、时序和观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
