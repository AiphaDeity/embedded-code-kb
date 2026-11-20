# 写入试跑记录，未确认就回到旧 App

两份 App 分别链接到 A 和 B。初次启动选 A，按 PA0 请求 B。Boot 在跳转前写下「已试跑 B」，再启动看门狗。B 运行满一秒后再次按键才能确认。没有确认就复位时，Boot 回到 A。B 确认后也能反向请求试跑 A。

## Flash 布局

| 区域 | 开始地址 | 用途 |
|---|---|---|
| 扇区 0 | 0x08000000 | Boot |
| 扇区 1 | 0x08004000 | 追加状态日志 |
| A 槽 | 0x08008000 | 扇区 2～5，描述符在槽起点，向量表在 0x08008200 |
| B 槽 | 0x08040000 | 扇区 6，描述符在槽起点，向量表在 0x08040200 |

STM32F407VET6 的两个槽在同一片内部 Flash 中，各有独立的可擦除扇区。候选镜像由 SWD 预装，示例从请求试跑开始。镜像 CRC 用于检测损坏。

## 沿执行路径读代码

1. App 在确认态收到有效按下就写入请求并重启。日志每条依次写状态、CRC 和提交字。半写记录在下次扫描时跳过。
2. Boot 按各自链接地址验证镜像。只有已确认槽可启动、候选也通过校验并且试跑标记成功持久化时，才跳入候选 App。
3. Boot 启动 IWDG 后把向量表、栈和复位入口交给 App。试跑 App 运行满一秒且 LED 已翻转后接受确认。确认持久化以后才喂狗。
4. 缺失确认的复位仍能读到「已试跑」。Boot 选择旧槽并尝试写回确认态。日志满时拒绝新请求，能够继续启动已确认槽。

## 完整源码

<a href="../../../examples/stm32/boot-rollback/boot/main.c" download="boot-main.c">下载 Boot</a> · <a href="../../../examples/stm32/boot-rollback/app/main.c" download="app-main.c">下载 App</a> · <a href="../../../examples/stm32/boot-rollback/policy.h" download="policy.h">下载状态机</a> · <a href="../../../examples/stm32/boot-rollback/state_flash.h" download="state_flash.h">下载 Flash 状态日志</a> · <a href="../../../examples/stm32/boot-rollback/layout.h" download="layout.h">下载布局</a> · <a href="../../../examples/stm32/boot-rollback/check.c" download="check.c">下载主机检查</a>

<<< ../../../examples/stm32/boot-rollback/boot/main.c

<<< ../../../examples/stm32/boot-rollback/app/main.c

镜像格式及切换 MSP 的汇编与[单槽示例](verify-and-jump.md)共用。

## 编译和观察

准备 GNU Arm、CMake、Ninja、Python 3 和 STM32CubeF4 V1.28.3，并替换 SDK 路径。

```sh
cmake -S examples/stm32/boot-rollback -B .work/sky-boot-rollback -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-boot-rollback
```

在专用实验板上用 SWD 写入 `boot/sky-rollback-boot.elf`，并将 `slot-a-header.bin`、`slot-a.bin`、`slot-b-header.bin`、`slot-b.bin` 分别写到上表的槽起点和向量表地址。首次验证使用已擦除的状态扇区，各次下载保留其他已写区域。

1. 复位后 A 槽的 PB2 每 500 ms 翻转。按 PA0 请求 B，B 应以 150 ms 翻转。
2. 先做未确认路径。等待 IWDG 复位，应恢复 A 的节拍。IWDG 标称期限约 8 秒，实际受 LSI 频率影响。
3. 再次试跑 B，在运行满一秒后且超时前按键。随后手动复位，B 应继续运行。调试时若冻结了 IWDG，请取消冻结后再验收自动回退。

## 练习

**预测题**　Boot 已写下「已试跑 B」，还没跳转就掉电，重启后会怎样？

<details><summary>核对预测</summary>

状态记录已经提交，而 B 未确认。下一次启动按已试跑处理，从 A 恢复。

</details>

**改动题**　只给 B 提供 CRC 不匹配的数据，再从 A 请求试跑。

<details><summary>核对改法</summary>

Boot 会在完整性检查拒绝 B，继续启动 A。可观察 integrity_b 和 selected。

</details>

## 验证记录

已生成 Boot 与 A/B 的三份 ELF，并核对向量表地址。主机用真实打包产物验证槽地址、CRC、试跑提交顺序、逆向回退和确认后的选择。看门狗复位及实际烧录待实板核对。[ST RM0090 参考手册](https://www.st.com/resource/en/reference_manual/dm00031020-stm32f405-415-stm32f407-417-stm32f427-437-and-stm32f429-439-advanced-arm-based-32-bit-mcus-stmicroelectronics.pdf)
