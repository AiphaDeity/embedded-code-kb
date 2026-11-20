# 嵌入式代码模板库

读一段完整代码，预测它在边界条件下的行为，再改一处验证自己的理解。

[打开知识库网页](https://aiphadeity.github.io/embedded-code-kb/)

## 可读示例

收到三个字节 `11 22 33`。从第二个字节开始能读出什么？如果只剩最后一个字节，输出变量会怎样？

| 从哪一步读 | 练习 | 已做验证 |
|---|---|---|
| [从字节数组的指定偏移读取 16 位整数](docs/c/byte-array-parsing.md) | 预测偏移 2 的结果，改成小端解析 | 电脑编译运行 |
| [天空星 GPIO：按住按键点亮 LED](docs/stm32/gpio/button-led.md) | 读按键电平，再改成每按一次切换 | 目标固件编译链接，待实板 |
| [天空星 RCC：GPIO 时钟使能和 AHB 分频](docs/stm32/rcc/clock-gates.md) | 对比三份时钟快照，预测 AHB 四分频的结果 | 目标固件编译链接，待实板 |
| [天空星 UART：USART1 轮询接收与回显](docs/stm32/uart/polling-echo.md) | 单字节回显、等待超时、改成固定四字节回显 | 目标固件编译链接，待实板 |
| [天空星用 UART DMA 连续接收两批不定长数据](docs/stm32/uart-dma-rx-idle.md) | 找到第二次接收在哪里启动 | 目标固件编译链接，待实板 |
| [在天空星上用 FreeRTOS 队列交接串口数据](docs/freertos/queue-message.md) | 让消费者放慢，观察队列满时的行为 | 目标固件编译链接，待实板 |

## 按运行环境浏览

初次读 STM32 工程可先走一遍[复位到最小主循环](docs/stm32/startup/minimal.md)。STM32 的 13 个编号均可打开最小外设示例。UART 可以按轮询、中断、DMA 连着读。[外设与总线目录](嵌入式代码模板库-总目录.md#外设与总线)

GPIO、USART1 和 TIM2 提供 HAL 与 CMSIS 寄存器两种实现。可以继续沿 [PWM 与输入捕获](docs/stm32/tim/pwm-capture.md)、[ADC 循环 DMA](docs/stm32/adc/timer-dma.md) 或 [CAN 双节点](docs/stm32/can/two-node.md) 阅读外设之间的交接。

综合练习可以顺着数据路径读。[ADC 到 JSON](docs/projects/sensor-to-json.md) · [SHT30 到 LCD](docs/projects/sht30-display.md) · [CAN 到 SD 卡](docs/projects/can-to-sd.md) · [排障索引](docs/guides/troubleshooting.md)

| 入口 | 内容 |
|---|---|
| [通用 C / C++](嵌入式代码模板库-总目录.md#common) | 字节解析、缓冲区、协议编解码与状态机 |
| [MCU 裸机 · STM32](嵌入式代码模板库-总目录.md#baremetal) | 外设、HAL 与寄存器写法，含 13 个外设与总线专题 |
| [MCU + RTOS · FreeRTOS](嵌入式代码模板库-总目录.md#rtos) | 中断与任务交接、队列、共享资源与运行诊断 |
| [嵌入式 Linux](嵌入式代码模板库-总目录.md#linux) | 用户态设备访问、事件监听、网关与驱动 |

[查看完整选题目录](嵌入式代码模板库-总目录.md)

## 在电脑上运行

首篇示例只需 C11 编译器，文章内附 GCC 命令。阅读本地网页版时，在装有 Node.js 22 或 24 的电脑上进入仓库根目录，执行以下命令并打开终端显示的网址。

```sh
npm ci
npm run docs:dev
```

要检查实际发布的静态产物，在同一目录构建并预览。

```sh
npm run docs:build
npm run docs:preview
```

站点路径为 `/embedded-code-kb/`，输出位于 `.vitepress/dist/`。站内「下载源码」链接使用随网站一并构建的 `examples/`。
