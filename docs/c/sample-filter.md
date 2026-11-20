# 算四点滑动平均并给阈值报警加滞回

输入是 0～4095 的 ADC 计数。平均值达到 3000 时告警，降到 2700 才解除。主循环每拿到一个有效采样就更新一次。

## 沿执行路径读代码

1. 先核对输入范围，防止无效采样进入窗口。
2. 维护样本总和。窗口填满后，写入新样本之前减掉被覆盖的旧值。
3. 用当前样本数求整数平均，跨过对应阈值时更新告警，再把平均值和告警一起交给调用者。

## 完整源码

<a href="../../examples/c/sample-filter/main.c" download="main.c">下载示例 main.c</a>

<<< ../../examples/c/sample-filter/main.c

## 编译和观察

在仓库根目录用 PowerShell 运行。Linux 下去掉输出文件的 `.exe` 后缀，并用 `./.work/sample-filter` 启动。

```powershell
New-Item -ItemType Directory -Force .work | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/c/sample-filter/main.c -o .work/sample-filter.exe
& ./.work/sample-filter.exe
```

程序输出 `sample filter: warm-up, wrap, hysteresis, invalid sample`。前四次平均依次为 3200、2900、2800、2750，告警保持。第五次覆盖旧值后平均变为 2600，告警解除。

## 练习

**预测题**　告警解除后，平均值升到 2973 会重新告警吗？

<details><summary>核对预测</summary>

2973 在两阈值之间，沿用解除状态。再升到 3000 才会开启。

</details>

**改动题**　把解除阈值改成 2800，记录在哪一次解除。

<details><summary>核对改法</summary>

第三次采样的整数平均为 2800，正好满足 `average <= WARN_OFF`。同步调整第三、第四次自测的告警预期。

</details>

## 验证记录

已在 Windows 使用 GCC 15.2.0 编译并运行自测。
