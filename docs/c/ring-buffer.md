# 环形缓冲区：写满、读空和索引回绕

用四个字节的定长空间缓存输入。写满后拒绝新数据，读走两项后继续写入，验证先进先出的顺序。

## 执行顺序

1. `count` 记录有效数据个数，`read` 和 `write` 各指向下一次操作的位置。
2. `ring_push()` 判断是否已满，成功才写入并推进。
3. `ring_pop()` 判断是否为空，成功才更新调用者的输出。两边推进后都按容量回绕。

## 完整源码

<a href="../../examples/c/ring-buffer/main.c" download="main.c">下载示例 main.c</a>

<<< ../../examples/c/ring-buffer/main.c

## 构建与观察

从仓库根目录编译并运行。在 Windows 的 PowerShell 中使用 `.work/ring-buffer.exe`，Linux 或 macOS 可把可执行文件名改为 `.work/ring-buffer`。

```powershell
New-Item -ItemType Directory -Force .work | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/c/ring-buffer/main.c -o .work/ring-buffer.exe
& ./.work/ring-buffer.exe
```

程序依次验证满时拒绝、索引回绕、FIFO 顺序和空时保留输出值，成功后打印 `ring buffer: OK`。这些函数由同一执行上下文调用。若给 ISR 和任务共享，须另外定义并发和内存可见性约定。

## 练习

**预测题**　缓冲区装满时，读写下标会相等吗？

<details><summary>核对预测</summary>

可以相等。由 `count` 区分全满与全空。

</details>

**改动题**　把容量改成 5。同步调整测试里的填充次数和预期读取序列。

<details><summary>核对改法</summary>

推进采用取模，可以使用非 2 的幂的容量。测试应仍覆盖满、空与绕回。

</details>

## 验证记录

已在 Windows 主机用 GCC 15.2.0 编译并运行全部自测，程序返回 0。
