# 05 · DMA 半传输中断交接已完成的前半缓冲区

让 DMA2 每轮搬运 16 个 32 位字。半传输回调发布前 8 字已完成，全传输回调发布后 8 字已完成。主循环分别验证两段，并负责启动下一轮。

## 沿执行路径读代码

1. `init_dma()` 配置内存到内存、源和目标地址递增、32 位数据以及正常模式，注册半传输、全传输和出错回调。
2. 主循环准备源数据，然后用 `HAL_DMA_Start_IT()` 启动。源和目标都在 DMA 可访问的普通 SRAM，本轮源数据保持有效。
3. `DMA2_Stream0_IRQHandler()` 交给 HAL。两个回调只发布各自范围的完成标记。主循环处理前半段时允许 DMA 继续写后半段。即使两个中断先于主循环到达，正常模式仍保留两段结果，直到主循环开始下一轮。
4. `wait_and_compare()` 用整轮共享的 100 ms 等待上限。超时、传输错误或数据不一致时保留 `result`，必要时中止搬运，然后停下。

DMA 的中断和正常模式见 [ST RM0090 DMA 章节](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)。循环模式需要额外判断何时会重写已交接的区域，可对照 [TIM2 触发 ADC 循环 DMA](../adc/timer-dma.md)。

## 完整源码

<a href="../../../examples/stm32/dma/half-transfer/main.c" download="main.c">下载 main.c</a> · <a href="../../../examples/stm32/dma/half-transfer/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../../examples/stm32/dma/half-transfer/main.c

## 构建与观察

```sh
cmake -S examples/stm32/dma/half-transfer -B .work/sky-dma-half-transfer -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-dma-half-transfer
```

通过 SWD 加载 `.work/sky-dma-half-transfer/sky-dma-half-transfer.elf`，从复位运行。

使用天空星 STM32F407VET6，本例只搬板上 SRAM。正常时 `verified_first`、`verified_second` 和 `batches_ok` 随运行增加。可能在同一轮看到前半计数先加一次。`result` 为 `COPY_PENDING` 表示一轮刚开始，为 `COPY_OK` 表示当前结果已比对通过。

## 练习

**预测题**　主循环因其他工作延迟，执行检查时 DMA 已完成整轮，第一半还能读吗？

<details><summary>核对预测</summary>

可以。正常模式结束后缓冲不会被自动写第二轮。主循环依次借助两个完成标记检查两段，再重新准备源和目标。

</details>

**改动题**　将每半区长度改为 16 字，计算传给 `HAL_DMA_Start_IT()` 的新长度。

<details><summary>核对改法</summary>

将 `HALF_WORDS` 设为 16，`WORDS` 就是 32。数据宽度仍为 32 位，传输数为 32 个元素，对应 128 字节。

</details>

## 验证记录

使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 交叉编译链接。用 QEMU Cortex-M4 运行到没有 DMA 完成事件的超时分支，读到 `result=TIMEOUT`，两个已验证半区和成功批次数均为 0。实际 DMA 搬运、回调顺序和 SRAM 内容待天空星实板验证。
