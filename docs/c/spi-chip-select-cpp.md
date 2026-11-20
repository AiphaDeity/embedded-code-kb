# 用 C++ 作用域管理阻塞式 SPI 片选

一次事务发送 `05`，接着给外设八个时钟取回状态。把片选释放写进对象析构，在正常返回、命令失败和回读失败时从同一位置退出片选。

## 沿执行路径读代码

1. 调用者传入一个有效的 `SpiPort`。`ChipSelect` 构造时选中设备，片选对象独占这次事务。
2. 端口完成命令和回读后，函数才更新输出。端口内部须保证一次阻塞收发已经结束才返回。
3. 离开函数时执行析构，片选恢复。移植到驱动时，端口实现可把 `select(true)` 映射为 GPIO 低电平。示例假定上层已保证 SPI 总线的独占。

## 完整源码

<a href="../../examples/cpp/spi-chip-select/main.cpp" download="main.cpp">下载示例 main.cpp</a>

<<< ../../examples/cpp/spi-chip-select/main.cpp

## 编译和观察

在仓库根目录用 PowerShell 运行。Linux 下去掉输出文件的 `.exe` 后缀，并用 `./.work/spi-chip-select` 启动。

```powershell
New-Item -ItemType Directory -Force .work | Out-Null
g++ -std=c++17 -Wall -Wextra -Werror -pedantic examples/cpp/spi-chip-select/main.cpp -o .work/spi-chip-select.exe
& ./.work/spi-chip-select.exe
```

程序输出 `SPI transaction: success and both failures release CS`。模拟端口检查收发发生在片选有效期间，并分别注入第一次、第二次收发失败。

## 练习

**预测题**　第一字节发送失败后，还会发送占位字节吗？

<details><summary>核对预测</summary>

`||` 在左侧为真时短路。函数直接返回，局部对象析构后释放片选。

</details>

**改动题**　在成功返回的用例中，改成由从机返回 `0x80`。

<details><summary>核对改法</summary>

改模拟端口回读值以及主程序的成功断言，两个失败用例应该仍保留原先的输出变量值。

</details>

## 验证记录

已在 Windows 使用 G++ 15.2.0 编译并运行模拟端口自测。
