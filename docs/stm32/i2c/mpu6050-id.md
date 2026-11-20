# 07 · I²C 读取 MPU6050 器件 ID

用 100 kHz I²C 读取 WHO_AM_I（寄存器 `0x75`）。成功时在调试器看到 `id=0x68`。

## 板型和接线

裸天空星核心板，外接 3.3 V 兼容 MPU6050 模块。PB6 → SCL，PB7 → SDA，电源和地按模块手册连接，AD0 接低。两根总线都要有到 3.3 V 的上拉电阻。

[TDK MPU6050 寄存器手册](https://www.invensense.tdk.com/en-us/download-resource/rm-mpu-6000a-00-mpu-6000-and-mpu-6050-register-map-and-descriptions)

在 Linux 上访问同一芯片可以对照 [I2C_RDWR 读取 MPU6050 温度](../../linux/i2c-mpu6050.md)。

确认器件 ID 后，可以在同一接线上继续做[连续读取三轴加速度](mpu6050-acceleration.md)。

## 执行顺序

1. 打开 GPIOB 和 I2C1，PB6/PB7 配 AF4 开漏。
2. 初始化控制器并等待器件上电。七位地址 `0x68` 左移一位传给 STM32 HAL。
3. `HAL_I2C_Mem_Read()` 指定一字节寄存器地址和接收长度。成功后再更新 `id`。

## 完整源码

<a href="../../../examples/stm32/i2c/mpu6050-id/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/i2c/mpu6050-id/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包路径替换为本机路径，在仓库根目录执行：

```sh
cmake -S examples/stm32/i2c/mpu6050-id -B .work/sky-i2c-mpu6050-id -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-i2c-mpu6050-id
```

用实验板通过 SWD 加载 `.work/sky-i2c-mpu6050-id/sky-i2c-mpu6050-id.elf` 并复位运行。正常时 `status=HAL_OK` 且 `id=0x68`。断电后把 AD0 改接高，使模块地址改变，原代码通常会报 `HAL_I2C_ERROR_AF`（无应答）。接线改动应在断电时完成。

## 练习

**预测题**　为什么设备地址要左移一位，寄存器 `0x75` 却保持原值？

<details><summary>核对预测</summary>

HAL 的设备地址参数预留了读写位。寄存器地址是 I²C 数据阶段发出的完整字节。

</details>

**改动题**　模块断电后将 AD0 拉高，同时把设备地址改成 `0x69`，重新读取。

<details><summary>核对改法</summary>

通信目的地址由 AD0 决定。WHO_AM_I 保存的标识值仍为 `0x68`。

</details>

## 验证记录

已使用 STM32CubeF4 V1.28.3 和 GNU Arm 14.3.1 编译及链接。接线、时序和观察值待实板验证。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
