# Linux poll 监听管道与 Unix 域套接字

## 示例

<a href="../../examples/linux/poll-ipc/main.c" download="main.c">下载示例源码</a>

<<< ../../examples/linux/poll-ipc/main.c

## 执行和验证

父进程通过同一次 `poll()` 等子进程的读数和状态。读数走 Unix 域数据报，状态走管道。只使用本地进程间通道。

```sh
mkdir -p .work
cc -std=c11 -Wall -Wextra -Werror examples/linux/poll-ipc/main.c -o .work/poll-ipc
./.work/poll-ipc
```

预期输出包含 `reading=42` 和 `status=D`，输出先后取决于事件何时到达。读完后移除对应的描述符；父进程最后用 `waitpid()` 接收子进程的退出状态。

**预测题**　写端发送一个字节后立即关闭，父进程为什么仍有机会读到该字节？

**改动题**　让子进程提前退出，只发送读数，观察父进程状态。

<details><summary>核对改法</summary>

读取端可能同时看到 `POLLIN` 和 `POLLHUP`，此时优先消费已经排队的数据。缺少状态时，管道挂断且无可读数据，父进程报错退出。

</details>

[Linux poll 手册](https://www.man7.org/linux/man-pages/man2/ppoll.2.html)

## 验证记录

已在 Alpine 3.22.8 的 WSL 环境编译并运行，核对两个事件及子进程正常退出。
