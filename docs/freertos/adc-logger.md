# FreeRTOS 把 ADC 采样与串口日志放进不同任务

采样任务每 100 ms 读一次 PC0，将序号、完成 tick 和原始值交给日志任务。暂时停掉日志消费，采样仍运行。待发队列满时丢本次新记录，恢复后可从日志序号看见缺口。

## 板型和接线

使用天空星 STM32F407VET6。电位器两端接板上模拟参考电压和 GND，中间滑动端接 PC0，接线按 [ADC 单通道例程](../stm32/adc/single-channel.md)。PA9 接 3.3 V USB 转串口的 RX，共地。终端用文本显示，115200、8N1、无流控。

## 沿执行路径读代码

1. `main()` 配好 ADC1 的规则通道 10、USART1 与四槽队列，再启动优先级较高的采样任务和日志任务。
2. `sample_pc0()` 每轮先取得一个序号。`convert()` 启动一次转换，等 EOC 后读取原始值，停止成功才把结果交给任务。失败时只计 `sample_errors`，下一周期继续。
3. 成功的记录由 `xQueueSend()` 按值复制。等待时间为 0 tick，所以队列满时采样任务立即返回，增加 `dropped_logs`。发送成功后采样任务可以复用局部变量。
4. `write_logs()` 取出自己的副本，再格式化、通过 UART 发送一行。格式和发送失败计入 `log_errors`。日志任务不访问 ADC。

输出从左到右是 `序号,采样完成 tick,ADC 原始值`。本工程每个 tick 为 1 ms。序号和 tick 都是 32 位无符号数，持续运行时会回绕。

## 完整源码

<a href="../../examples/freertos/adc-logger/main.c" download="main.c">下载 main.c</a> · <a href="../../examples/freertos/adc-logger/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a> · <a href="../../examples/stm32/common/sky-adc1-pc0.h" download="sky-adc1-pc0.h">下载 ADC1 初始化</a> · <a href="../../examples/stm32/common/sky-usart1.h" download="sky-usart1.h">下载 USART1 初始化</a>

<<< ../../examples/freertos/adc-logger/main.c

## 构建与观察

```sh
cmake -S examples/freertos/adc-logger -B .work/sky-freertos-adc-logger -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-freertos-adc-logger
```

通过 SWD 加载 `.work/sky-freertos-adc-logger/sky-freertos-adc-logger.elf`，从复位运行。

转动电位器，用串口终端观察第三列随输入变化。调试器中 `raw_samples` 统计成功转换，`logged` 统计发送成功。正常运行时通常两者接近；队列积压期间先出现差值。

## 练习

**预测题**　已经有四条待打日志，采样又成功一次。哪一条会丢掉？

<details><summary>核对预测</summary>

新的一条丢掉，原先四条仍保持顺序。`raw_samples` 和 `dropped_logs` 各加一。恢复后的日志序号可以定位缺失的区间。

</details>

**改动题**　用调试器将 `pause_logger` 设为 1，让程序连续运行约一秒，再改回 0。

<details><summary>核对改法</summary>

暂停期间比较 `raw_samples`、`logged` 和 `dropped_logs`。恢复后观察先发出的积压记录和其后新采样的序号，核对序号仍递增且中间有缺口。

</details>

## 验证记录

目标固件已交叉编译链接。在 QEMU 运行原固件，ADC 模型未提供本例等待的 EOC，测得转换失败时 `sample_errors` 增长且无日志。另用单独的模拟测试构建，让 `HAL_ADC_PollForConversion()` 返回成功来补入完成事件，再用 QEMU 的 ADC 读数和实际 FreeRTOS 队列、USART1 检查日志。暂停期间样本累计从 3 到 23，丢弃 16 条；恢复后接收到序号 4～7，再收到 24～27。模拟读数与板上电压无关。天空星的 EOC、实际采样值和串口节拍待实板验证。
