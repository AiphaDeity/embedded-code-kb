# C 把有符号原始值换算为定点读数

从有符号采样计数算出毫摄氏度，支持比例、零点偏移和限幅。比如每计数 `1/16 °C`，原始值 16 经 `125 m°C` 补偿后得到 `1125 m°C`。

## 沿执行路径读代码

1. `apply_scale()` 先检查配置和输出指针。分母必须为正，最小值不能大于最大值。失败时保留上一次读数。
2. 将原始计数扩为 `int64_t` 后乘比例分子，再用商与余数舍入。恰好半个单位时远离 0，所以 `±62.5 m°C` 得到 `±63 m°C`。
3. 舍入后加入偏移，以 64 位结果与量程比较。先完成限幅，最后把结果缩窄成 32 位输出。

例如希望输出摄氏度的千分之一，`numerator=1000`、`denominator=16` 表示每计数对应 `62.5 m°C`。`offset_mc=125` 表示在换算后再加 `0.125 °C`。来源为传感器时，先按其手册确认比例和单位。

## 完整源码

<a href="../../examples/c/fixed-point-scale/main.c" download="main.c">下载 main.c</a>

<<< ../../examples/c/fixed-point-scale/main.c

## 编译和观察

在仓库根目录使用 C11 编译器。PowerShell 命令如下。

```powershell
New-Item -ItemType Directory -Force .work | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/c/fixed-point-scale/main.c -o .work/fixed-point-scale.exe
& ./.work/fixed-point-scale.exe
```

输出 `raw=16 -> 1125 mC`。自检还覆盖负数、半单位、整数极值、限幅和无效分母。

## 练习

**预测题**　当前配置加 `125 m°C` 偏移，原始值 `-1` 输出多少？

<details><summary>核对预测</summary>

先将 `-62.5 m°C` 舍入为 `-63 m°C`，加偏移后为 `62 m°C`，仍在量程内。

</details>

**改动题**　使用原配置和 `125 m°C` 偏移，补一个 `raw=2000` 的断言。

<details><summary>核对改法</summary>

计算得到 `125125 m°C`，超过 `max_mc=125000`。成功返回时 `reading` 为 `125000`。

</details>

## 验证记录

Windows GCC 和隔离主机环境均通过编译、自测。核对正负半单位、两端限幅、非法配置保留旧值以及两个整数乘法极限。
