# 09 · TIM2 定时中断翻转 LED

每 500 ms 翻转一次 LED。一个完整的亮灭周期约为 1 秒。

可继续读 [CMSIS 寄存器版](register-periodic-led.md) 或 [PWM 输出与输入捕获测频](pwm-capture.md)。

## 板型和接线

使用天空星 PB2 板载 LED，连接供电和 SWD。示例从复位默认 HSI 启动，APB1 分频保持 /1。

## 执行顺序

1. 确认 TIM2 输入时钟为标称 16 MHz。PSC=15999 得到 1 kHz 计数频率。
2. ARR=499 使每 500 个计数产生一次更新。启动前清除初始化可能产生的更新标志。
3. TIM2 IRQ 经 HAL 转到周期回调。回调先计数，再翻转 PB2。

## 完整源码

<a href="../../../examples/stm32/tim/periodic-led/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/tim/periodic-led/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包路径替换为本机路径，在仓库根目录执行：

```sh
cmake -S examples/stm32/tim/periodic-led -B .work/sky-tim-periodic-led -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-tim-periodic-led
```

用实验板通过 SWD 加载 `.work/sky-tim-periodic-led/sky-tim-periodic-led.elf` 并复位运行。LED 约亮 0.5 秒、灭 0.5 秒。调试器中 `updates` 按每秒约两次增加。实测可用示波器探 PB2。

## 练习

**预测题**　为什么间隔是 500 个时钟而不是 499 个？

<details><summary>核对预测</summary>

计数器从零开始并计到 ARR，包含 0 和 499。计算使用 `(PSC+1) × (ARR+1) / TIMxCLK`。

</details>

**改动题**　保持 PSC 不变，把 ARR 改成 249。

<details><summary>核对改法</summary>

每 250 ms 翻转一次，完整周期约 500 ms。若以后修改 APB1 分频，先查 TIM2 的实际输入时钟。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译及链接。接线、时序和观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
