# C 用配置结构体和回调上报状态变化

传入两条规则，连续失败 2 次才告警，连续成功 3 次才恢复。每次状态改变同步调用一次回调。可以为两个实例共用回调函数，并用各自的 `context` 把事件写到不同地方。

## 沿执行路径读代码

1. `health_init()` 检查阈值大于 0，成功后按值复制 `HealthConfig`。调用者可以随后修改原配置，已经创建的实例保留各自的配置。回调和 `context` 的指针值被复制，`context` 指向的对象需要在回调执行时有效。
2. `health_record()` 收到一次结果。若结果与当前健康状态一致，就清掉反方向的连续计数。达到另一个状态的阈值后，先更新状态和计数，再带着 `context` 调用 `on_change`。空回调仍会更新状态。
3. `record_change()` 把 `context` 转成 `EventLog *`。同样的函数分别写两个实例自己的日志。调用者串行提交结果，回调不再次调用同一实例。

## 完整源码

<a href="../../examples/c/health-callback/main.c" download="main.c">下载 main.c</a>

<<< ../../examples/c/health-callback/main.c

## 编译和观察

在仓库根目录执行。需要本机 GCC。

```powershell
New-Item -ItemType Directory -Force .work | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/c/health-callback/main.c -o .work/health-callback.exe
& ./.work/health-callback.exe
```

输出两次 `alarm`，分别来自链路和传感器，随后打印一次 `recovered`。自测另外检查非法配置失败后保留原状态、连续成功中断后重计以及空回调行为。

## 练习

**预测题**　当前已告警，接着收到「成功、成功、失败、成功」。能恢复吗？

<details><summary>核对预测</summary>

仍处于告警。失败把前两次成功清零，最后一次成功重新计到 1，距离恢复还差连续两次成功。

</details>

**改动题**　令传感器也在第一次失败时报错，保持链路使用原来的独立日志。

<details><summary>核对改法</summary>

传给传感器的 `failures_to_alarm` 改为 1，并更新第一条记录后的断言。现在第一次调用 `health_record(&sensor, false)` 会使 `first.calls` 为 1，`second.calls` 仍为 0。随后给链路提交一次失败，`second.calls` 才变成 1。

</details>

## 验证记录

使用 Windows MSYS2 UCRT64 GCC 15.2.0，以 `-O2 -Wall -Wextra -Werror -pedantic` 编译并运行。输出两次 `alarm` 和一次 `recovered`，自测通过。
