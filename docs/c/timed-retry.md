# 在主循环里处理超时、计数回绕与有限重试

每次主循环向状态机交当前时间和已匹配的应答，再根据返回的动作发请求或报告结果。每轮最多等待 20 ms，总共最多发 3 次。

## 沿执行路径读代码

1. 第一次调用发出请求，记录本轮起点。主循环可以在两次调用之间做别的工作。
2. 用 `uint32_t` 的 `now - started_at` 算经过时间。严格小于 20 ms 才属于应答窗口，恰好 20 ms 按到期处理。
3. 超时且剩余次数可用时，把重发动作交给调用方。所有次数用完后仅上报一次，直到调用者创建新的命令。

## 完整源码

<a href="../../examples/c/timed-retry/main.c" download="main.c">下载示例 main.c</a>

<<< ../../examples/c/timed-retry/main.c

## 编译和观察

在仓库根目录用 PowerShell 运行。Linux 下去掉输出文件的 `.exe` 后缀，并用 `./.work/timed-retry` 启动。

```powershell
New-Item -ItemType Directory -Force .work | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/c/timed-retry/main.c -o .work/timed-retry.exe
& ./.work/timed-retry.exe
```

程序输出 `timed retry: wrap, reply, 3 attempts, single timeout`。计数从 `UINT32_MAX - 9` 开始，回绕后的 `9` 距起点 19 ms，`10` 距起点 20 ms。

## 练习

**预测题**　为什么 `now` 变小后不会继续等到下次计数回到原值？

<details><summary>核对预测</summary>

时间差按无符号计数的模数计算。本例只关心短时间窗口，并要求轮询间隔小于完整回绕周期。

</details>

**改动题**　把最大发送次数改成 2，更新完全无应答的用例。

<details><summary>核对改法</summary>

保留 0 ms 和 20 ms 两次发送。40 ms 就报告超时，之后继续调用只得到 `NOTHING`。

</details>

## 验证记录

已在 Windows 使用 GCC 15.2.0 编译并运行自测。
