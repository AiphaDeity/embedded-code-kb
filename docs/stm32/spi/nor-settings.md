# 用 SPI NOR 双槽保存参数并回退到上次提交

W25Q128 的两个 4 KiB 扇区分别保留一份设置。换槽时先擦除备用扇区，读回主体后最后写提交字。掉电重启从有效记录中选比较新的代数。

## 板型与接线

选择专用 3.3 V W25Q128JV 模块，PA4/PA5/PA6/PA7 接 CS/SCK/MISO/MOSI。PA0 触发保存，PB2 观察闪灯。实验预留 0xFFE000 与 0xFFF000 两个扇区，接入前确认其内容可覆盖。

## 沿执行路径读代码

1. SPI 方向固定为 Mode 0。读 JEDEC ID，核对 EF 40 18，再加载两个记录。
2. 每次更新都重新读槽，擦除较旧的一侧。页编程前确认写使能，擦除、编程后都等待器件退出 busy。
3. 先比对主体，最后写提交字。下一次启动核对范围、CRC 和提交标记，再以无符号差选择较新记录。

## 完整源码

<a href="../../../examples/stm32/spi/nor-settings/main.c" download="main.c">下载示例 main.c</a>

<<< ../../../examples/stm32/spi/nor-settings/main.c

<a href="../../../examples/stm32/spi/nor-settings/store.h" download="store.h">下载双槽算法</a> · <a href="../../../examples/stm32/spi/nor-settings/check.c" download="check.c">下载主机检查</a> · <a href="../../../examples/c/common/crc32.h" download="crc32.h">下载 CRC-32</a> · <a href="../../../examples/c/debounce/debounce.h" download="debounce.h">下载按键消抖</a>

<<< ../../../examples/stm32/spi/nor-settings/store.h{c}

## 编译和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3。将固件包目录换成本机路径后，在仓库根目录运行以下命令。

```sh
cmake -S examples/stm32/spi/nor-settings -B .work/sky-spi-nor-settings -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-spi-nor-settings
```

第一次按键以 100 ms 间隔翻灯。再按变 200 ms，active_slot 在两个地址间交替。按核心板复位键时保持外接模块供电，核对恢复的间隔和 generation。

## 练习

**预测题**　备用槽只写完主体，电源就断了，启动会用哪个参数？

<details><summary>核对预测</summary>

提交字尚未生效，启动继续选择原有效槽。

</details>

**改动题**　用主机检查程序把切断位置改到提交字的第二个字节前。

<details><summary>核对改法</summary>

保留旧参数。只有写全提交字且主体 CRC 正确，才会选新记录。

</details>

## 验证记录

主机内存模型逐字节切断 16 个写入位置，覆盖坏 CRC、读失败禁止擦除和代数回绕。固件已编译链接，实际芯片时序及中途掉电待实测。 [Winbond W25Q128JV 产品与数据手册](https://www.winbond.com/hq/product/code-storage-flash/qspi-nor/w25q-jv/?__locale=en&partNo=W25Q128JVSIQG)

运行独立主机检查。

```powershell
New-Item -ItemType Directory -Force .work | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/stm32/spi/nor-settings/check.c -o .work/spi-nor-check.exe
& ./.work/spi-nor-check.exe
```
