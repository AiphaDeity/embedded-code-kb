# FreeRTOS 用总期限等待命令应答

一个任务排队提交输入 `42`，另一个任务计算并回传 `127`。客户端接受序号一致且在 80 ms 内收到的结果。上一轮迟到的应答会被丢弃，本轮仍按原来的截止时间等待。

## 沿执行路径读代码

1. `main()` 创建两个深度为 2 的队列，分别存请求和应答。两队列都按值复制，发送方可以复用局部变量。
2. `client()` 每轮分配一个序号，记下 `started`，再尝试入队。队列满就记 `request_drops`，本轮不等待。
3. `worker()` 阻塞等请求。奇数序号模拟 15 ms，偶数序号模拟 125 ms。完成后把原序号和结果写入应答队列，满时只记 `reply_drops`。
4. `await_reply()` 每次用 **80 tick 减去已过 tick** 作为阻塞时间。收到其他序号，就记 `old_replies`，重新计算剩余时间。到 80 tick 即结束。只有按时且同号的结果可以写到 `latest`。

从空队列开始，命令 1 约 15 ms 完成。命令 2 提交后等了 80 ms，客户端超时，再等 20 ms 就提交命令 3。此时 Worker 还在处理 2。约在命令 3 提交后第 25 ms，客户端收到并丢掉 2 的应答，再等约 15 ms 收到 3 的应答，仍在期限内。这里的超时结束客户端的等待，Worker 会继续完成已经接手的命令。

## 完整源码

<a href="../../examples/freertos/command-deadline/main.c" download="main.c">下载示例 main.c</a> · <a href="../../examples/freertos/command-deadline/CMakeLists.txt" download="CMakeLists.txt">下载构建文件</a>

<<< ../../examples/freertos/command-deadline/main.c

## 构建和观察

天空星 STM32F407VET6 供电后接 SWD 调试器，无需外接总线。使用 GNU Arm、CMake、Ninja、STM32CubeF4 V1.28.3，把固件包路径替换成自己的路径。

```sh
cmake -S examples/freertos/command-deadline -B .work/sky-freertos-command-deadline -G Ninja -DCUBE_F4_ROOT="D:/SDK/STM32Cube_FW_F4_V1.28.3"
cmake --build .work/sky-freertos-command-deadline
```

通过 SWD 加载 `.work/sky-freertos-command-deadline/sky-freertos-command-deadline.elf` 并复位。运行一段时间后暂停，看 `latest.id` 是否为奇数、`latest.value` 是否为 127。取两次快照，比较 `accepted`、`timed_out` 和 `old_replies` 是否各自增加。

## 练习

**预测题**　若在收到过期应答后重新等满 80 ms，命令 3 最长可能等多久？

<details><summary>核对预测</summary>

这个例子中，旧应答约在命令 3 提交后 25 ms 来。如果它触发一次全新的 80 ms 等待，而正确应答一直没到，命令 3 最多可能等约 105 ms。继续使用同一个 `started`，丢掉旧应答后只剩约 55 ms。

</details>

**改动题**　把偶数序号的模拟耗时从 125 ms 改为 35 ms，复位后比较计数。

<details><summary>核对改法</summary>

只改 `worker()` 里的耗时，保留 80 ms。两种请求在默认调度下都应按时完成，`accepted` 持续增长，`timed_out` 和 `old_replies` 预期为 0。多暂停几次，`latest.id` 可以看到奇数和偶数。

</details>

## 验证记录

已用 GNU Arm 14.3.1 和 STM32CubeF4 V1.28.3 编译链接。QEMU Cortex-M4 一次快照中，接受 78 次、超时 77 次、丢弃旧序号 77 次，请求与应答队列均无丢弃。最近一次接受的结果是 `{155, 127}`。天空星上观察这些计数待验证。
