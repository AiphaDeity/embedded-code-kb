# 天空星 Boot 校验 App 镜像并交接启动

Boot 读取 App 描述符，检查镜像长度、栈顶、Thumb 入口和 CRC，再切换向量表并进入 App 的 `Reset_Handler`。实验在 STM32F407VET6 天空星上用 PB2 区分两个分支，Boot 校验失败时常亮，App 启动后每 250 ms 翻转一次。

## Flash 分区

| 分区 | 起点 | 内容 |
|---|---:|---|
| Boot | `0x08000000` | 第一扇区，16 KiB，存放 Boot 固件 |
| 描述符 | `0x08004000` | 第二扇区，存放 16 字节 App 描述符 |
| App | `0x08008000` | 从第三扇区起，最多 480 KiB，镜像从向量表开始 |

描述符按小端存放 4 个字段，依次是 `KBA1`、格式版本、镜像字节数和 CRC-32。CRC 检测误码和写入损坏。镜像来源认证、升级状态和回退属于后续升级设计。

## 从 Boot 读到 App

1. 上电复位后，Cortex-M 从 Boot 向量表取初始 MSP 与复位入口。STM32CubeF4 的 `Reset_Handler` 调 `SystemInit()`，复制 `.data`、清零 `.bss`，再经 C 运行时进入 Boot 的 `main()`。
2. Boot 先用向量检查和 CRC 验证可接受的镜像，再关闭 SysTick 和外部中断，清除待处理中断。这个 Boot 从复位默认时钟进入，交接前未启动 DMA 或其他外设请求。
3. Boot 将 `SCB->VTOR` 指向 App，汇编入口装载 App MSP 并跳转。App 的 `SystemInit()` 使用同一个向量表地址，启动代码重新初始化它自己的 `.data` 和 `.bss`。
4. App 的 `main()` 配置 PB2 和 SysTick。观察 `half_period_ms` 初始为 250、`ticks` 和 `toggles` 递增，可以沿新向量表追到 App 自己的中断处理。

## 完整源码

### Boot 与镜像校验

<a href="../../../examples/stm32/boot-app/boot/main.c" download="boot.c">下载 Boot 源码</a>

<<< ../../../examples/stm32/boot-app/boot/main.c

<a href="../../../examples/stm32/boot-app/boot/image.h" download="image.h">下载镜像校验头文件</a>
· <a href="../../../examples/c/common/crc32.h" download="crc32.h">下载共用 CRC-32 函数</a>

<<< ../../../examples/stm32/boot-app/boot/image.h{c}

### 向量交接

<a href="../../../examples/stm32/boot-app/boot/enter_app.S" download="enter_app.S">下载交接汇编</a>

<<< ../../../examples/stm32/boot-app/boot/enter_app.S{asm}

<a href="../../../examples/stm32/boot-app/boot/handoff.h" download="handoff.h">下载中断清理与向量表切换</a>。同一路径也用于 [A/B 试跑与回滚](trial-rollback.md)。

### App

<a href="../../../examples/stm32/boot-app/app/main.c" download="app.c">下载 App 源码</a>

<<< ../../../examples/stm32/boot-app/app/main.c

### 打包

<a href="../../../examples/stm32/boot-app/pack.py" download="pack.py">下载打包脚本</a>

<<< ../../../examples/stm32/boot-app/pack.py

## 编译、验证和烧录

准备 GNU Arm、CMake、Ninja、Python 3 和 STM32CubeF4 V1.28.3。在仓库根目录运行以下命令，替换固件包路径。

```sh
cmake -S examples/stm32/boot-app -B .work/sky-boot-app -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-boot-app
```

输出目录包含 Boot 和 App 的 ELF、map 文件、`app-header.bin` 和 `app.bin`。启用主机 GCC 后，可用 <a href="../../../examples/stm32/boot-app/check.c" download="check.c">check.c</a> 校验同一份交叉构建产物。

```powershell
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/stm32/boot-app/check.c -o .work/sky-boot-app/check-image.exe
& ./.work/sky-boot-app/check-image.exe .work/sky-boot-app/app-header.bin .work/sky-boot-app/app.bin
```

用 `arm-none-eabi-objdump -h` 查看 ELF。Boot 和 App 的 `.isr_vector` 运行地址分别是 `0x08000000` 和 `0x08008000`。在 <a href="../../../examples/stm32/common/stm32f407ve.ld" download="stm32f407ve.ld">链接脚本</a> 和 `sky-app.map` 中追 App 的 `.data`，`_sdata` 是 SRAM 里的运行地址，`_sidata` 是 Flash 上的初始值地址。App 的启动代码按这两个地址完成复制。

在专用实验板上使用 SWD 确认已准备空白的相应分区。按 Boot ELF 的装载地址写入，再把 `app-header.bin` 写到 `0x08004000`，`app.bin` 写到 `0x08008000`。逐个核对下载地址和校验结果，后一次写入保留前一次写入的扇区。复位后预计 LED 闪动。在校验失败的用例里，可在 `show_failure()` 下断点看 `boot_result`。

## 练习

**预测题**　只改 App 代码，重新生成 App 后沿用旧描述符，会停在哪一步？

<details><summary>核对预测</summary>

App 长度、入口或 CRC 有差异时，Boot 停在对应校验分支。正常更新要烧入由同一次构建生成的描述符和镜像。

</details>

**改动题**　把 App 的 `half_period_ms` 改成 500，再构建并运行主机校验。

<details><summary>核对改法</summary>

打包工具会用新的 App 内容重新算 CRC。更新匹配的 App 与描述符后，实板 LED 完整周期预计从约 500 ms 变为约 1 秒。

</details>

## 验证记录

已用 GNU Arm 14.3.1 编译链接两份 ELF，核对向量表地址和 `enter_app` 反汇编，并用主机检查程序验证实际生成的 App 镜像、CRC 被改、Thumb 位缺失、文件截短和描述符损坏。SWD 烧录与天空星实测待补。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/rm0090-stm32f407-advanced-armbased-32bit-mcus-stmicroelectronics.pdf)
