# 用稳定时间过滤按键抖动

每轮把原始电平和毫秒计数交给状态机。候选电平持续 20 ms 后才报告边沿。可用同一状态机在[天空星按键翻灯](../stm32/gpio/debounced-button.md)。

## 完整源码

<a href="../../examples/c/debounce/main.c" download="main.c">下载主机示例</a> · <a href="../../examples/c/debounce/debounce.h" download="debounce.h">下载状态机</a>

<<< ../../examples/c/debounce/debounce.h{c}

<<< ../../examples/c/debounce/main.c

## 运行和观察

```powershell
New-Item -ItemType Directory -Force .work | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/c/debounce/main.c -o .work/debounce.exe
& ./.work/debounce.exe
```

自测把最后一次抖动放在时基回绕之前。候选状态跟踪每次观察到的变化，并用无符号差计算等待时间。

**预测题**　保持高电平继续调用，会一直收到按下事件吗？

<details><summary>核对预测</summary>

候选已和稳定值一致，继续返回 KEY_IDLE。等到稳定松开后，才可能报告下一次按下。

</details>

**改动题**　把等待时间改成 10 ms，预测回绕后首次有效报告。

<details><summary>核对改法</summary>

最后一次上升设在 UINT32_MAX - 9。回绕时刻 0 正好经过 10 ms，此时再次调用可收到 KEY_PRESSED。

</details>

## 验证记录

已在 Windows 上用 GCC 15.2.0 严格编译并运行，验证抖动、计数回绕、单次按下和单次释放。
