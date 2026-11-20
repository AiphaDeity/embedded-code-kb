# 05 · DMA2 搬运内存并比对

用 DMA2 Stream0 把四个 32 位数搬到另一个数组。CPU 等传输返回后逐字比较，可以在调试器看到搬运数据和结果。

需要让主循环分别处理前后两个半区时，见 [DMA 半传输中断](half-transfer.md)。

## 板型和接线

天空星核心板，接供电和 SWD。使用普通 SRAM 中的两个 32 位数组，默认链接脚本将它们放入 DMA2 可访问的地址范围。

## 执行顺序

1. 选择 DMA2、内存到内存、FIFO、普通模式。源地址走外设侧地址寄存器，因此两边的自增和宽度都要配置。
2. 调用 `HAL_DMA_Start()` 后轮询传输完成。长度参数是搬运的元素数量。
3. 传输完成后比对两端，给出 `result`。失败时可查 HAL 状态和错误码。

## 完整源码

<a href="../../../examples/stm32/dma/memory-copy/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/dma/memory-copy/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包路径替换为本机路径，在仓库根目录执行：

```sh
cmake -S examples/stm32/dma/memory-copy -B .work/sky-dma-memory-copy -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-dma-memory-copy
```

用实验板通过 SWD 加载 `.work/sky-dma-memory-copy/sky-dma-memory-copy.elf` 并复位运行。`result=1` 代表目标数组匹配。负值分别表示初始化、启动、等待或比对失败。观察 `destination` 中的四个数和 DMA Stream 的 NDTR。

## 练习

**预测题**　为什么长度传 4，实际搬了 16 字节？

<details><summary>核对预测</summary>

两端的粒度都是 32 位，4 表示四个元素。

</details>

**改动题**　将源数组中一个元素替换为 `0xA5A55A5A`，重建后在目标数组定位同一个元素。

<details><summary>核对改法</summary>

地址自增开启后，改变的值应出现在目标数组的同一个下标。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译及链接。接线、时序和观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
