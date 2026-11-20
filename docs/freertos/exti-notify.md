# FreeRTOS 从 EXTI 中断直接通知任务

天空星 PA0 每个上升沿经 EXTI0 增加一次通知。任务取走累计值，再决定 PB2 LED 是否翻转。任务处理上一批期间继续到来的边沿可以合成下一批。

## 板型和接线

用天空星 STM32F407VET6 的板载 PA0 用户键和 PB2 LED，接供电和 SWD 即可。此例直接统计上升沿，机械按键可能一次按下出现多个边沿。需要按键事件防抖时可继续读[裸机 EXTI 边沿计数](../stm32/exti/button-irq.md)。

## 沿执行路径读代码

1. `main()` 创建消费任务并保存句柄。调度启动后，`button_task()` 再开放 EXTI0，旧挂起先清除。NVIC 数值优先级为 6，满足当前 FreeRTOS 配置允许调用内核的边界 5。
2. `EXTI0_IRQHandler()` 让 HAL 检查并清除挂起位。回调将 `irq_edges` 增一，再调用 `vTaskNotifyGiveFromISR()`。唤醒更高优先级任务时，`portYIELD_FROM_ISR()` 会请求中断返回后切换。
3. `ulTaskNotifyTake(pdTRUE, portMAX_DELAY)` 阻塞直到有通知，得到旧通知值并清零。任务把整个值加到 `handled_edges`，本批为奇数时翻转 PB2，偶数次翻转的净效果是保持原状态。
4. 任务延时 50 ms 模拟其他工作。延时中到达的通知仍然累积，下一次 `ulTaskNotifyTake()` 可以直接取走。每次 ISR 要先清硬件挂起位，后一个边沿才能作为新事件被处理。

## 完整源码

<a href="../../examples/freertos/exti-notify/main.c" download="main.c">下载 main.c</a> · <a href="../../examples/freertos/exti-notify/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../examples/freertos/exti-notify/main.c

## 构建和观察

准备 GNU Arm、CMake、Ninja 和 STM32CubeF4 V1.28.3，替换固件包路径。

```sh
cmake -S examples/freertos/exti-notify -B .work/sky-freertos-exti-notify -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-freertos-exti-notify
```

通过 SWD 加载 `sky-freertos-exti-notify.elf`。观察 `irq_edges`、`handled_edges`、`batches` 和 `max_batch`。要控制注入次数，可以给 EXTI0 的软件触发寄存器写 bit 0。每次写前确认上一轮的 EXTI0 挂起已清除，否则两个触发可能共用一次挂起。

## 练习

**预测题**　第一次取走 1 之后，任务延时期间又发生三次 ISR。延时结束时取到几？

<details><summary>核对预测</summary>

第二批取到 3，`batches` 为 2，`handled_edges` 增到 4，`max_batch` 为 3。两批都为奇数，LED 各翻转一次。

</details>

**改动题**　把消费改成每次只处理一条通知，再注入同样的边沿序列。

<details><summary>核对改法</summary>

将第一个参数改为 `pdFALSE`，每次成功从通知值减 1。返回值仍是减之前的累计数，因此把处理数量固定为 1，返回值只用于判断是否取到。原样延时 50 ms 后，四个通知应分别经历四轮消费。

</details>

## 验证记录

GNU Arm 14.3.1 编译链接通过。以 QEMU EXTI 输入注入 1 个边沿，再在消费前注入 3 个。两轮处理后，ISR 边沿数为 4、消费数为 4、批次数为 2，最后一批为 3。
