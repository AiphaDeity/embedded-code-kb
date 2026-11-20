# 07 · I²C 连续读取 MPU6050 三轴加速度

天空星从 MPU6050 的 `0x3B` 起一次读六字节，转换为 X、Y、Z 三个有符号 16 位原始值。导线、地址或传输异常时，可同时查看 HAL 返回值和错误位。

## 板型和接线

使用天空星 STM32F407VET6 与 3.3 V 兼容 MPU6050 模块。PB6 接 SCL，PB7 接 SDA，两线通过上拉接 3.3 V，电源按模块要求连接并共地。AD0 接低时用 7 位地址 `0x68`，传给 HAL 时左移一位。

## 沿执行路径读代码

1. `bus_init()` 配置 I²C1 为 100 kHz。第一次循环先读 `WHO_AM_I`，只有 `0x68` 才写配置。
2. `prepare_sensor()` 唤醒芯片并设置 ±2 g。下一步 `read_acceleration()` 写寄存器地址，再通过重复起始转成读方向，连续取六字节。该寄存器块会为一次连续读提供同一采样时刻的数据。
3. `accel_decode()` 把每轴的高、低字节按补码还原。比如 `40 00 C0 00 00 01` 解码为 `16384, -16384, 1`。本量程下 `16384` 个计数约为 `1 g`。
4. 完整读回才更新 `latest`。失败时保留上次样本并清 `sample_valid`，主循环随后重读身份并配置。持续故障时，可看 `phase`、`outcome`、`last_hal_status` 和 `last_hal_error`。

寄存器、输出格式和数据同步见 [TDK MPU6050 寄存器表](https://www.invensense.tdk.com/en-us/download-resource/rm-mpu-6000a-00-mpu-6000-and-mpu-6050-register-map-and-descriptions)。接口语义见 [STM32F4 HAL 手册](https://www.st.com/resource/en/user_manual/um1725-description-of-stm32f4-hal-and-lowlayer-drivers-stmicroelectronics.pdf)。

## 完整源码

<a href="../../../examples/stm32/i2c/mpu6050-acceleration/main.c" download="main.c">下载 main.c</a> · <a href="../../../examples/stm32/i2c/mpu6050-acceleration/acceleration.h" download="acceleration.h">下载数据处理头文件</a> · <a href="../../../examples/stm32/i2c/mpu6050-acceleration/check.c" download="check.c">下载主机检查</a> · <a href="../../../examples/stm32/i2c/mpu6050-acceleration/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../../examples/stm32/i2c/mpu6050-acceleration/main.c

<<< ../../../examples/stm32/i2c/mpu6050-acceleration/acceleration.h{c}

## 构建与观察

```sh
cmake -S examples/stm32/i2c/mpu6050-acceleration -B .work/sky-mpu6050-acceleration -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-mpu6050-acceleration
```

通过 SWD 加载 `.work/sky-mpu6050-acceleration/sky-mpu6050-acceleration.elf`，从复位运行。

传感器静止时，沿重力方向的轴原始值通常接近 `16384` 或 `-16384`，另外两轴接近零，具体值受安装姿态与偏移影响。改变方向后，对比 `latest`、`good_samples` 和 `sample_valid`。

| 观察到的结果 | 判断依据 |
|---|---|
| `IO_NACK` | HAL 错误位含 `HAL_I2C_ERROR_AF`。逻辑分析仪可以分辨是哪一字节之后没有 ACK。 |
| `IO_TIMEOUT` | HAL 返回 `HAL_TIMEOUT`，或错误位含 `HAL_I2C_ERROR_TIMEOUT`。例如等不到地址阶段所需事件。 |
| `IO_BUSY` | HAL 返回 `HAL_BUSY`，有可能在真正发起地址之前发现总线一直忙。 |
| `BAD_ID` | 事务成功，但读取的身份与 MPU6050 不符。示例跳过后续寄存器写入。 |

## 练习

**预测题**　X 轴读出 `FF FF`，它代表多少个原始计数？

<details><summary>核对预测</summary>

得到 `65535` 后按补码减 `65536`，结果为 `-1` 个计数。把这两个字节直接当无符号数会把接近零的负数读成大正数。

</details>

**改动题**　保持 AD0 接低，将 `ADDRESS` 改用 `0x69 << 1`。读取身份时记录逻辑分析仪和程序状态。

<details><summary>核对改法</summary>

如果总线没有其他器件响应 `0x69`，地址阶段预计无 ACK，`phase` 为 `IDENTITY` 且没有有效样本。恢复地址后再比较 `good_samples`。线路一直忙时观察到的可能是 `IO_BUSY`，应先排查接线和上拉。

</details>

## 验证记录

用 STM32CubeF4 V1.28.3 与 GNU Arm 14.3.1 交叉编译链接。主机通过正负轴、极值和 `-1` 解码检查。QEMU 没有模拟该板的 I²C 外设，实际运行确认身份读取会走 `IO_TIMEOUT`，保留 HAL 错误位且不发布样本。总线应答、连续采样与实测数值待天空星验证。

```sh
mkdir -p .work
cc -std=c11 -Wall -Wextra -Werror examples/stm32/i2c/mpu6050-acceleration/check.c -o .work/mpu-accel-check
.work/mpu-accel-check
```
