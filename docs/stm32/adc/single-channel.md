# 06 · ADC1 轮询采样电位器

每隔约 100 ms 读取一次 PC0，先观察 12 位原始值，再观察按参考电压换算的毫伏值。

进一步可用 [TIM2 触发 ADC1 与循环 DMA](timer-dma.md) 代替主循环逐次发起采样。

## 板型和接线

采用天空星核心板的 PC0（ADC1_IN10）。10 kΩ 电位器两端接 3.3 V 和 GND，中间端接 PC0。共地。用筑基板时可用其 PC0 电位器，先核对板卡配置。

[天空星引脚资料](https://wiki.lckfb.com/zh-hans/web-tool/fdb-pinout.html)

## 执行顺序

1. 开启 GPIOC、ADC1，把 PC0 配为模拟输入。
2. ADC1 配为单通道、12 位、软件触发、单次转换。选择足够长的采样时间。
3. 启动后轮询等待 EOC。成功才取原始值，记录采样状态并等待下一轮。

## 完整源码

<a href="../../../examples/stm32/adc/single-channel/main.c" download="main.c">下载示例 main.c</a> · <a href="../../../examples/stm32/common/sky-adc1-pc0.h" download="sky-adc1-pc0.h">下载共用 ADC1 初始化</a>

<<< ../../../examples/stm32/adc/single-channel/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包路径替换为本机路径，在仓库根目录执行：

```sh
cmake -S examples/stm32/adc/single-channel -B .work/sky-adc-single-channel -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-adc-single-channel
```

用实验板通过 SWD 加载 `.work/sky-adc-single-channel/sky-adc-single-channel.elf` 并复位运行。转动电位器，`raw` 随电压变化，范围为 0～4095。`millivolts` 使用 3000 mV 的参考值估算，若输入超过实际 VREF+，读数饱和。根据核心板版本量 VREF+ 后再改常量。

## 练习

**预测题**　如果原始值一直是 4095，应先检查什么？

<details><summary>核对预测</summary>

检查 PC0 的电压和板上 VREF+，再确认没有把模拟输入和数字口混接。

</details>

**改动题**　把电位器旋到中间，比较万用表测得的 PC0 电压与 `millivolts`。

<details><summary>核对改法</summary>

以 VREF+ 约 3.0 V、3.3 V 供电为例，中间约 1.65 V，对应原始值约 2252。电位器位置、电源和参考误差会改变实测值。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译及链接。接线、时序和观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
