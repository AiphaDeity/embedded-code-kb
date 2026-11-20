# 字节流解析：半包、连续帧和校验失败

解析帧头 `AA`、一字节长度、负载和 XOR 校验。把输入拆成多次送入，识别完整帧并在错误后回到找帧头。

## 执行顺序

1. 找帧头后检查长度，接受 1～8 字节。
2. 只在负载状态把每个字节写入数组，并更新校验。
3. 收齐负载后再验证校验字节。成功时由调用者消费数据，失败时恢复寻找新帧。

## 完整源码

<a href="../../examples/c/stream-frame/main.c" download="main.c">下载示例 main.c</a>

<<< ../../examples/c/stream-frame/main.c

解帧逻辑放在与项目示例共用的头文件中。

<<< ../../examples/c/stream-frame/decoder.h{c}

## 构建与观察

从仓库根目录编译并运行。在 Windows 的 PowerShell 中使用 `.work/stream-frame.exe`，Linux 或 macOS 可把可执行文件名改为 `.work/stream-frame`。

```powershell
New-Item -ItemType Directory -Force .work | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/c/stream-frame/main.c -o .work/stream-frame.exe
& ./.work/stream-frame.exe
```

自测包含前导噪声、拆开的帧、校验失败和后续有效帧，成功打印 `stream parser: 2 frames`。XOR 是用于练习的校验。生产协议应按实际规范判断完整性和重新同步。

## 练习

**预测题**　负载中的 `AA` 需要立刻当作新帧头吗？

<details><summary>核对预测</summary>

解析器处于读取负载状态时，`AA` 是普通数据。越过一个确认有效的长度之后，状态才转到校验。

</details>

**改动题**　把最后一个有效帧的校验改错，然后重新运行。

<details><summary>核对改法</summary>

有效帧数变成 1，末尾自测会失败。把测试用例预期改成 1 后，仍可检查错误路径。

</details>

## 验证记录

已在 Windows 主机用 GCC 15.2.0 编译并运行全部自测，程序返回 0。
