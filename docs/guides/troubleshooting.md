# 调试、构建和板端排障索引

| 现象 | 先看哪个证据 | 对应示例 |
|---|---|---|
| 固件编译通过，板上像没启动 | 确认目标型号、ELF 下载地址和复位后的 PC；在 `main()` 下断点 | [复位到最小主循环](../stm32/startup/minimal.md) |
| 固件进入 HardFault | 读取 CFSR、HFSR 与有效帧中的 PC，用本次 ELF 定位源码 | [HardFault 快照](../stm32/fault/hardfault-snapshot.md) |
| Boot 校验失败或 App 未启动 | 核对镜像长度、CRC、向量表地址、栈顶和复位入口，再查看 VTOR 与 App 的 SysTick | [Boot 到 App](../stm32/boot-app/verify-and-jump.md) |
| GPIO 配置后电平不变 | Watch 先比对两个端口的时钟使能位，再看引脚模式和有效电平 | [RCC 快照](../stm32/rcc/clock-gates.md) |
| 串口只收一次 | 判断本次回调是否执行，确认是否重挂下一轮，再看交接槽丢弃计数 | [中断回显](../stm32/uart/interrupt-echo.md) · [DMA 接收](../stm32/uart-dma-rx-idle.md) |
| I²C 无应答 | 核对七位地址、AD0、共地、上拉和 HAL 错误码。用逻辑分析仪确认地址阶段 | [器件 ID](../stm32/i2c/mpu6050-id.md) |
| 按一下出现两次事件 | 同时观察原始电平、候选值与稳定值，确认事件只在候选持续够久时发布 | [按键消抖](../stm32/gpio/debounced-button.md) |
| SD 卡挂载或追加失败 | 挂载或文件失败先看 log_status，再看 sd_error、CLK 和 CMD | [SDIO 与 FatFs](../stm32/sdio/fatfs-log.md) |
| 串口收到字节，但没有 JSON | 用已知 5 字节试输入，检查分层失败点。确认字段长度、字节序和校验 | [采样到 JSON](../projects/sensor-to-json.md) |
| PWM 频率或占空比偏差 | 核对 TIM 时钟、PSC、ARR 和 CCR，再用输入捕获比对测量周期 | [PWM 与输入捕获](../stm32/tim/pwm-capture.md) |
| CAN 回环通过、外部对端没收到 | 依次检查收发器供电、终端电阻、波特率与过滤。内部回环用于验证控制器路径 | [CAN 回环](../stm32/can/internal-loopback.md) · [CAN 双节点](../stm32/can/two-node.md) |
| 网络恢复后积压没有发送 | 先查本地队首记录，再核对服务端是否落库、回的 ACK 是否匹配 | [TCP 缓存补传](../linux/tcp-outbox.md) |
| 看门狗反复复位 | 一开机先快照复位标志，再看是否及时进入喂狗循环 | [IWDG 复位](../stm32/iwdg/reset-cause.md) |

## 每次记录一组可复现证据

1. 保存用到的目标板、SDK、编译命令和实际加载的 ELF 文件名。链接输出能证明镜像生成。
2. 连接 SWD 后先查启动和寄存器状态。测串口、I²C、SPI 或定时器时，优先用计数和波形确定时序，再在必要的位置下断点。
3. 写明实际输入和观察值。例如“每次收到一字节后重新挂起接收，回传第三字节后错误计数仍为零”。这样另一台板可以重复同一组实验。

## 先看 ELF，再测板上信号

以 [SHT30 温湿度显示](../projects/sht30-display.md) 的 ELF 为例。安装 Arm GNU 工具链后，在仓库根目录执行。

```sh
arm-none-eabi-objdump -h .work/sky-sht30-display/sky-sht30-display.elf
arm-none-eabi-gdb --batch .work/sky-sht30-display/sky-sht30-display.elf \
  -ex 'info address main' -ex 'info address last_sample' -ex 'ptype last_sample'
```

第一条命令核对向量表起点，当前构建的 `.isr_vector` 位于 `0x08000000`。GDB 可读出 `main`、`last_sample` 的地址和类型。编译配置变化后重新查看本次 ELF 的地址。

接入 SWD 后先确认能进入 `main()`，再运行整轮采样，停下观察 `sample_status`、`samples_ok` 和 `samples_failed`。需要测总线时，先取消时序路径中的断点，让程序自由运行。

| 工具 | 接在哪一步 | 在 SHT30 示例中看什么 |
|---|---|---|
| GDB / SWD | 启动、初始化和一轮结束 | 从 `main` 进入主循环，核对状态、计数和 `last_sample` |
| 逻辑分析仪 | SCL / SDA，共地 | 解码 7 位地址 `0x44`，看 `24 00`、两次事务间隔和随后的六字节读取 |
| 示波器 | SCL / SDA 的上升沿、供电与地 | 核对上拉后的电平、上升时间和采样期间的供电变化 |

这些 ELF 检查已在电脑上运行。SWD、逻辑分析仪和示波器的观察值留待实板记录。

## 练习

**预测题**　如果 `.isr_vector` 出现在预期地址，现在能确定 `main()` 已运行吗？

<details><summary>核对预测</summary>

这里只确认了 ELF 布局。可以在复位入口和 `main()` 各下一个断点，进一步区分镜像启动、运行时初始化和应用入口。

</details>

**改动题**　把上面第一条命令的文件换成 [Boot/App 实验](../stm32/boot-app/verify-and-jump.md)生成的 Boot 和 App ELF，分别读向量表地址。

<details><summary>核对改法</summary>

常规 Boot 的 `.isr_vector` 位于 `0x08000000`，该示例 App 位于 `0x08008000`。电脑上已用两个实际构建产物读回这组地址。

</details>
