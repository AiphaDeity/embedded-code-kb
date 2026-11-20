# 从字节数组的指定偏移读取 16 位整数

串口交给你三个十六进制字节 `11 22 33`。协议规定要读的字段从第二个字节开始，占两个字节，高位在前。这时偏移是 1，字段值应为 `0x2233`。先沿着 `main()` 中的第一次调用读进 `read_u16_be()`，再试着解释为什么第三次调用会失败。

## 可直接运行的 C 示例

想先手推两字节如何合成一个整数，可读[第 16 课 · 字节序](../c-reading/16.md)。

<a href="../../examples/c/byte-array-parsing/main.c" download="main.c">查看或下载完整源码 main.c</a>

<<< ../../examples/c/byte-array-parsing/main.c

在仓库根目录运行。Windows PowerShell 示例使用 GCC：

```powershell
gcc -std=c11 -Wall -Wextra -Werror examples/c/byte-array-parsing/main.c -o byte-array-demo.exe
.\byte-array-demo.exe
```

Linux 或 macOS 使用 `cc -std=c11 -Wall -Wextra -Werror examples/c/byte-array-parsing/main.c -o byte-array-demo`，再运行 `./byte-array-demo`。程序逐次打印结果并检查预期值，相符时退出码为 0。

## 大端字段从哪一位读起

1. **先找长度的来源。** 在 `main()` 里，`data` 是一个含三个元素的数组，`sizeof(data)` 得到 3 个字节。进入函数后，`data` 是指针，实际可读长度由单独的 `length` 参数传入。在真实收包代码里应传入本次收到的有效字节数。
2. **再找第一次可能退出的位置。** 指针为空时直接返回。接着，`offset > length` 成立时，逻辑或运算符 `||` 会跳过右侧表达式。偏移合法时才计算 `length - offset`。这可以处理特别大的偏移，也避开了先计算 `offset + 2` 时可能发生的回绕。
3. **最后看写回位置。** 偏移 1 对应 `0x22`，左移 8 位得到 `0x2200`，再与下一个字节 `0x33` 按位或，得到 `0x2233`。`out` 指向调用者的 `value`，只有走到这一步才更新它。

## 用同一数组走五次

| 偏移 | 函数观察到的情况 | 返回 | 调用后 value |
|---|---|---|---|
| 0 | 从头取 `11 22` | true | `0x1122` |
| 1 | 末尾恰好还有 `22 33` | true | `0x2233` |
| 2 | 只剩一个字节 | false | `0x2233` |
| 3 | 偏移到达数组末尾 | false | `0x2233` |
| SIZE_MAX | 偏移超过数组长度 | false | `0x2233` |

如果失败后仍看到上一次的值，检查调用者是否先判断了布尔返回值。输出变量保留原值是这个函数的接口约定。

## 自测

**预测题**　把 `value` 初始化为 `0xCAFE`，跳过其他调用，直接用偏移 2 调用一次。返回值和输出值各是多少？

<details><summary>核对预测</summary>

返回 false，输出仍为 `0xCAFE`。前表中的 `0x2233` 来自上一次成功调用，失败分支没有给输出赋值。

</details>

**改动题**　在副本里把函数改成读小端字段。先写出偏移 0 和 1 的预期结果，再改组合表达式及样例的预期值。运行五组输入，确认三个失败用例仍然通过。

<details><summary>核对改法</summary>

第一个字节放低位，第二个字节放高位。将函数改名为 `read_u16_le` 并更新调用处，组合表达式改为 `(uint16_t)(((uint16_t)data[offset + 1U] << 8) | data[offset])`，预期数组改为 `{0x2211, 0x3322}`。长度和偏移的判断可以原样使用。

</details>

## 验证记录

Windows 主机使用 MSYS2 UCRT64 GCC 15.2.0，以 C11 模式加 `-Wall -Wextra -Werror -pedantic` 编译。大端示例的五组输出与上表一致，退出码为 0。另在副本中完成小端练习，前两次分别得到 `0x2211`、`0x3322`，后三次保留 `0x3322`，退出码同样为 0。
