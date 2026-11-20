# 08 · SPI 双 DMA 连续读取 16 字节

在一次片选有效期内发送 W25Q128 读命令和三字节地址，再用占位字节产生读取时钟。完成后在调试器中查看地址 0 起的 16 字节。

## 板型和接线

使用 3.3 V W25Q128 模块。PA4 接 CS，PA5 接 SCK，PA6 接 MISO，PA7 接 MOSI，并共地。按模块说明使 WP 和 HOLD 保持非激活。

## 沿执行路径读代码

1. SPI1 给 DMA2 的两个 Stream 发请求。Stream0 收，Stream3 发，均使用 Channel3。
2. 主循环拉低 CS 后启动全双工传输，TX 包含命令、地址与占位，RX 与它同步写入。完成回调把缓冲交给主循环。
3. 主循环释放 CS，跳过前四个响应位置。失败或超时先终止传输，再择机重试；无法终止时停止，保护 DMA 尚可能使用的缓冲。

## 完整源码

<a href="../../../examples/stm32/spi/dma-read/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/spi/dma-read/main.c

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3，将固件包路径换成本机路径。

```sh
cmake -S examples/stm32/spi/dma-read -B .work/sky-spi-dma-read -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-spi-dma-read
```

用 SWD 下载 `.work/sky-spi-dma-read/sky-spi-dma-read.elf`，复位运行。已知有内容的模块应读到对应数据。空白介质通常读到连续 FF。观察 success_count、failure_count 和 last_error，并用逻辑分析仪核对片选覆盖完整 20 字节。

## 练习

**预测题**　为什么 RX 的第一个字节不能当作地址 0 的内容？

<details><summary>核对预测</summary>

第一字节的时钟用于发命令，后面三个字节用于发地址。第四个位置之后才开始返回负载。

</details>

**改动题**　把读地址改成 0x000010，保持读长度不变。

<details><summary>核对改法</summary>

命令 buffer 的地址低字节设为 0x10。片选仍覆盖 20 字节，数据取新地址开始的一段。

</details>

## 验证记录

已用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译链接。信号与观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
