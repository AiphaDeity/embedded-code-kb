# Linux 网络断线后重试，收到确认再删除本地缓存

先把两条 ADC 读数写入 SQLite。服务没开时，发送端在有限重试后退出，待发记录仍在。服务恢复后按顺序发送。若某条记录已保存但应答在路上丢失，重试使用同一个消息 ID。

## 沿执行路径读代码

1. `enqueue()` 把消息 ID 和读数一起提交到本地数据库。再次启动发送进程时，`drain()` 从最早一项开始读。
2. 每条记录单独建立 TCP 连接，用换行标记记录末尾。连接、发送和接收各有期限。接收方用整条记录共用的期限处理分段到达的数据。
3. `receive_one()` 先把 ID 和读数提交到接收端数据库，再回送 `ACK <ID>`。相同 ID、相同值再次到达时仍会确认。ID 相同、读数不同的记录会被拒绝。
4. 发送端拿到匹配的 ACK 才删除本地行。断开、超时或者确认 ID 不匹配时保留该行，下次仍从它开始。

## 完整源码

<a href="../../examples/linux/tcp-outbox/main.py" download="main.py">下载示例 main.py</a>

<<< ../../examples/linux/tcp-outbox/main.py

## 在一台 Linux 机器上复现

使用带 SQLite 标准库的 Python 3。在仓库根目录开两个终端。发送端和接收端分别用不同的 SQLite 文件。本实验由单个发送进程按队首顺序补传。

终端 A 先写入两个读数。保持服务端关闭，执行发送。

```sh
mkdir -p .work/tcp-outbox-demo
python3 examples/linux/tcp-outbox/main.py enqueue .work/tcp-outbox-demo/pending.db 837
python3 examples/linux/tcp-outbox/main.py enqueue .work/tcp-outbox-demo/pending.db 1024
python3 examples/linux/tcp-outbox/main.py drain .work/tcp-outbox-demo/pending.db 17349 --attempts 2
```

这次预计退出码为 2，两行仍在本地。终端 B 开启实验服务器，并让接收端首次入库后故意不回 ACK。

```sh
python3 examples/linux/tcp-outbox/main.py serve .work/tcp-outbox-demo/seen.db 17349 --drop-first-ack
```

在终端 A 再发一次。观察接收端先打印 `ack dropped`，然后发送端重试并依次确认两条记录。

```sh
python3 examples/linux/tcp-outbox/main.py drain .work/tcp-outbox-demo/pending.db 17349
python3 -c "import sqlite3; db=sqlite3.connect('.work/tcp-outbox-demo/seen.db'); print(db.execute('SELECT raw FROM seen ORDER BY rowid').fetchall())"
```

接收端应有两条记录 `[(837,), (1024,)]`。再执行一次 `drain` 应直接返回 0。需要重复整套实验时换一个数据库目录。

## 练习

**预测题**　接收端完成数据库提交，发送端还没删除缓存时掉电，重启后会发生什么？

<details><summary>核对预测</summary>

下次仍会发送该 ID。接收端从 `seen` 查到同一个值，直接回 ACK，发送端随后删除待发记录。该去重约定覆盖例子中的数据库写入。若接收后还要驱动外部设备，需要给相应操作设计自己的幂等或提交机制。

</details>

**改动题**　服务器开启 `--drop-first-ack` 时，让首次发送只试一次。退出后再启动发送端。

<details><summary>核对改法</summary>

第一次 `drain` 增加 `--attempts 1`。它应退出 2。重新运行时读到同一条缓存，收到 ACK 后才会继续下一条。对比两个进程看到的消息 ID。

</details>

## 验证记录

在 Windows 和 Alpine 3.22.8 WSL 分别实跑 TCP 回环与 SQLite。验证了离线重试上限、接收端提交后丢 ACK、重启发送进程、依入队顺序补传、同 ID 去重，以及分片到达但载荷冲突的拒绝路径。

从仓库根目录执行 `python3 examples/linux/tcp-outbox/check.py` 可重跑这些检查。脚本使用临时目录和本地随机端口。
