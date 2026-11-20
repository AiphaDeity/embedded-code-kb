# 12 · IWDG 停喂后读取复位原因

正常时持续喂狗，按住用户键让主循环停止喂狗。下一次启动读取复位原因，并点亮板载 LED。

## 板型和接线

天空星核心板，使用 PA0 用户键和 PB2 LED。接 SWD 便于读 `reset_flags`。

## 执行顺序

1. 读取并清除 RCC 复位标志，保存本次启动看到的状态。
2. 按标称 32 kHz LSI、32 分频、1999 重装值配置约 2 秒。初始化后 IWDG 已开始计时。
3. 正常运行每 100 ms 喂狗。发现按键后停止喂狗，等待硬件复位。
4. 若复位标志表明来自 IWDG，点亮 LED 并继续喂狗，让结果可观察。

## 完整源码

<a href="../../../examples/stm32/iwdg/reset-cause/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/iwdg/reset-cause/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包路径替换为本机路径，在仓库根目录执行：

```sh
cmake -S examples/stm32/iwdg/reset-cause -B .work/sky-iwdg-reset-cause -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-iwdg-reset-cause
```

用实验板通过 SWD 加载 `.work/sky-iwdg-reset-cause/sky-iwdg-reset-cause.elf` 并复位运行。上电后 LED 熄灭。按住 PA0 超过一次 100 ms 轮询，预期约 2 秒后重启，`reset_flags & RCC_CSR_IWDGRSTF` 非零，LED 亮。LSI 有误差，秒数仅作估计。调试器停核期间的计时还取决于 DBGMCU 冻结设置。

## 练习

**预测题**　为什么要在清除复位标志之前保存快照？

<details><summary>核对预测</summary>

清除操作会把本次启动要看的硬件证据抹去。

</details>

**改动题**　把 `Reload` 改为 999，重新比较停喂后的等待时间。

<details><summary>核对改法</summary>

标称由约 2 秒变成约 1 秒。实际观察按板上 LSI 和调试器设置为准。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译及链接。接线、时序和观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
