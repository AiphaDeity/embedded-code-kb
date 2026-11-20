# 11 · DAC1 循环输出三档电压

用软件写入 12 位 DAC 数据，每 1 秒切换一档电压。在高阻表上可以直接看到电压变化。

## 板型和接线

使用天空星核心板 PA4（DAC_OUT1），万用表或示波器的信号端接 PA4、地接 GND。使用高阻测量，先确认 PA4 没有接继电器或其他负载。

## 执行顺序

1. PA4 配模拟模式，打开 DAC 外设时钟。
2. 配置 DAC 通道 1，软件写值，无外部触发，启用输出缓冲。
3. 依次写入 1024、2048、3072，成功才更新 `active_code`。

## 完整源码

<a href="../../../examples/stm32/dac/voltage-steps/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/dac/voltage-steps/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包路径替换为本机路径，在仓库根目录执行：

```sh
cmake -S examples/stm32/dac/voltage-steps -B .work/sky-dac-voltage-steps -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-dac-voltage-steps
```

用实验板通过 SWD 加载 `.work/sky-dac-voltage-steps/sky-dac-voltage-steps.elf` 并复位运行。标称 VREF+ 为 3.0 V 时，预期三档约 0.75 V、1.50 V、2.25 V。以手中板实际参考电压和测得的输出为准。

## 练习

**预测题**　为什么本例选中间三档？

<details><summary>核对预测</summary>

避开输出缓冲接近供电轨时的输出限制，比较比例更直观。

</details>

**改动题**　加一档 512，用万用表记录新电压。

<details><summary>核对改法</summary>

参考为 3.0 V 时，512 约为满量程的八分之一，预期约 0.375 V。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译及链接。接线、时序和观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
