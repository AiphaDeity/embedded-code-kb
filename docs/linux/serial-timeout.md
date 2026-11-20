# Linux 串口限时读取

## 示例

<a href="../../examples/linux/serial-timeout/main.c" download="main.c">下载示例源码</a>

<<< ../../examples/linux/serial-timeout/main.c

## 执行和验证

使用 `termios` 把设备配置为 115200 8N1，`poll()` 等待可读。有数据时按十六进制输出，1 秒没有数据时输出 `timeout`，退出码为 2。设备错误的退出码为 1。

在 Linux 仓库根目录运行：

```sh
mkdir -p .work
cc -std=c11 -Wall -Wextra -Werror examples/linux/serial-timeout/main.c -o .work/serial-timeout
./.work/serial-timeout /dev/ttyUSB0
```

先关掉抢占同一设备的串口工具。使用已接好共地的 3.3 V 适配器或配置正确的虚拟串口，核对设备名和访问权限。正常条件下对端可发送 `41 00 7E`。一次 `read()` 返回本次已经可读的字节数，连续调用才可能收齐完整报文。

**预测题**　如果发完前两个字节后停顿，程序会等待第三个字节吗？

**改动题**　将等待窗口改成 200 ms，用空闲设备重新运行。

<details><summary>核对改法</summary>

一次可读事件只负责读当前的数据，不会等待固定帧长。把 `poll()` 的第三个参数改为 200 后，空闲窗口约为 200 ms，正常超时仍返回 2。

</details>

[Linux termios 手册](https://man7.org/linux/man-pages/man3/termios.3.html) · [Linux poll 手册](https://www.man7.org/linux/man-pages/man2/ppoll.2.html)

## 验证记录

已在 Alpine 3.22.8 的 WSL 环境编译并运行。伪终端验证了空等超时、`41 00 7E` 的原样读取及退出后恢复原配置。实体串口与天空星接线待实测。
