# 给 Modbus RTU 请求追加 CRC-16 并验证

把 `01 03 00 00 00 0A` 写成一条完整请求，帧尾应追加 `C5 CD`。示例把 CRC 计算、帧尾写入和接收校验分开，调用方负责确定一帧的边界。

## 沿执行路径读代码

1. CRC 的初始值是 `FFFF`，每个字节先和 CRC 低八位异或。移出的最低位为 1 时，再和反射形式的多项式 `A001` 异或。
2. 先检查缓冲区可写长度，再把计算值的低、高字节依次写到负载之后。
3. 收到整帧后，把帧尾还原成 16 位值，与前面字节重算的结果比较。确认 CRC 后，调用方还要核对地址和功能码。

## 完整源码

<a href="../../examples/c/crc16-modbus/main.c" download="main.c">下载示例 main.c</a>
· <a href="../../examples/c/crc16-modbus/crc16.h" download="crc16.h">下载 CRC-16 函数</a>

<<< ../../examples/c/crc16-modbus/main.c

<<< ../../examples/c/crc16-modbus/crc16.h{c}

## 编译和观察

在仓库根目录用 PowerShell 运行。Linux 下去掉输出文件的 `.exe` 后缀，并用 `./.work/crc16-modbus` 启动。

```powershell
New-Item -ItemType Directory -Force .work | Out-Null
gcc -std=c11 -Wall -Wextra -Werror -pedantic examples/c/crc16-modbus/main.c -o .work/crc16-modbus.exe
& ./.work/crc16-modbus.exe
```

程序输出 `CRC-16/MODBUS: 4B37, request tail C5 CD`。自测覆盖缓冲不足、长度越界、完整请求、翻转负载位和短帧。

## 练习

**预测题**　计算值为 `0xCDC5` 时，线上先看到哪个字节？

<details><summary>核对预测</summary>

先看到低字节 `C5`，再看到高字节 `CD`。

</details>

**改动题**　把负载损坏测试换为翻转帧尾的最后一位。

<details><summary>核对改法</summary>

将 `request[2] ^= 0x01U` 改为 `request[7] ^= 0x01U`。负载重算的 CRC 保持原值，收到的高字节发生变化，所以结果仍是失败。

</details>

## 验证记录

已在 Windows 使用 GCC 15.2.0 编译并运行自测。
