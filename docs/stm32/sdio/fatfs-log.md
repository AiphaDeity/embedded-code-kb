# 用 SDIO 和 FatFs 向 SD 卡追加一行日志

每次复位挂载现有 FAT16/FAT32 卷，向 KBLOG.TXT 追加一行 sky: sd log，成功关闭文件后点亮 PB2。

## 板型和接线

使用 STM32F407VET6 和 3.3 V 原生 SD 接口。PC8 接 DAT0，PC12 接 CLK，PD2 接 CMD，并共地。按卡座要求给 CMD 和 DAT 线上拉。准备已有 FAT 卷的卡。

## 沿执行路径读代码

1. HSI PLLQ 给 SDIO 提供 48 MHz，HAL 用识别速率初始化，再切到 1 位传输。
2. FatFs 经 disk_read 和 disk_write 按 512 字节访问，写后等卡完成内部编程。
3. 以追加模式打开日志，核对实际字节数。关闭文件同步缓存，再卸载。

## 完整源码

<a href="../../../examples/stm32/sdio/fatfs-log/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/sdio/fatfs-log/main.c

SDIO 与 FatFs 之间的适配也用于 [CAN 记录项目](../../projects/can-to-sd.md)。<a href="../../../examples/stm32/sdio/fatfs-log/sdio_disk.c" download="sdio_disk.c">下载共用的 diskio 实现</a>

<a href="../../../examples/stm32/sdio/fatfs-log/logger.h" download="logger.h">下载追加逻辑</a> · <a href="../../../examples/stm32/sdio/fatfs-log/check.c" download="check.c">下载内存盘检查</a>

<<< ../../../examples/stm32/sdio/fatfs-log/logger.h{c}

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。在仓库根目录执行以下命令，替换固件包路径。

```sh
cmake -S examples/stm32/sdio/fatfs-log -B .work/sky-sdio-fatfs -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-sdio-fatfs
```

通过 SWD 加载 `.work/sky-sdio-fatfs/sky-sdio-fatfs.elf` 后复位。复位两次后，用电脑读卡应看到两条日志。挂载失败时 LED 熄灭，调试器读取 log_status 和 sd_error。

## 练习

**预测题**　f_write 返回 FR_OK，怎样判断整行写完？

<details><summary>核对预测</summary>

比较 written 与请求字节数。空间不足可能短写，示例将这种情况报告为 FR_DENIED。

</details>

**改动题**　修改追加文本后再复位。

<details><summary>核对改法</summary>

旧行保留，新文本在文件末尾。

</details>

## 验证记录

先用上面的 CMake 命令生成 FatFs 配置，再在 Windows 用原 SDK 的 `ff.c` 跑内存盘检查。把 `$fatfs` 换成本机 SDK 源码目录。

```powershell
$fatfs = 'D:/SDK/STM32Cube_FW_F4_V1.28.3/Middlewares/Third_Party/FatFs/src'
gcc -std=c11 -I.work/sky-sdio-fatfs "-I$fatfs" "$fatfs/ff.c" examples/stm32/sdio/fatfs-log/check.c -o .work/sky-sdio-fatfs/check.exe
& ./.work/sky-sdio-fatfs/check.exe
```

已用 GNU Arm 链接 SDIO 和 SDK 自带 FatFs。主机用同一 FatFs 验证空卷拒绝、两次追加、同步和读回。实体 SD 卡收发待验证。 [ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
