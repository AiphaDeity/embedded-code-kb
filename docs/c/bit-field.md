# C 用掩码读取和改写寄存器位域

给定一个 32 位字，把 bit[6:4] 改成 `5`，原字的其他位保留。示例使用内存变量，可以先在电脑上运行。

## 沿执行路径读代码

1. `field_mask()` 检查起始位置与字段宽度。宽为 3 时得到低位掩码 `0x07`，完整 32 位宽时直接使用 `UINT32_MAX`。
2. `change_field()` 判断新值能否放进字段，再把掩码和值左移到起始位置。先清除原字段，随后合并新值。
3. `read_field()` 右移并掩码。字段超出 32 位或新值过宽时，函数返回 `false`，输出参数保持原值。

在硬件中先确认寄存器允许读改写，并确保这次读写不会与中断或其他执行上下文争用。写 1 清除的挂起位采用直接写对应掩码的方式，例如 [EXTI 共享中断](../stm32/exti/shared-lines.md)。

## 完整源码

<a href="../../examples/c/bit-field/main.c" download="main.c">下载 main.c</a>

<<< ../../examples/c/bit-field/main.c

## 编译和观察

在仓库根目录使用 C11 编译器。PowerShell 命令如下。

```powershell
New-Item -ItemType Directory -Force .work | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/c/bit-field/main.c -o .work/bit-field.exe
& ./.work/bit-field.exe
```

应看到 `0xA5A500F3 -> 0xA5A500D3, field=5`。自检还核对过宽输入、超出最高位、全宽和第 31 位。

## 练习

**预测题**　向三位字段写入 8，函数会得到什么结果？

<details><summary>核对预测</summary>

三位最大值是 7，`8 & ~0x07` 非零。`change_field()` 返回 `false`，原字保留。

</details>

**改动题**　把 bit[6:4] 的新值改成 2，重新跑自测。

<details><summary>核对改法</summary>

原低字节 `0xF3` 先清掩码 `0x70`，得到 `0x83`，再合并 `0x20`。更新读回断言及两处完整字比较，结果应为 `0xA5A500A3`。

</details>

## 验证记录

Windows GCC 和隔离主机环境均通过编译、自测。含越界位移和完整 32 位边界。
