# Linux 交叉构建、部署和运行日志

先将串口程序在开发机编译，再把适合目标架构的产物集成到板端镜像。用相同的输入比较本机和目标机的输出，可以判断问题出在程序还是部署环境。

## 构建与确认架构

```sh
make -C examples/linux
file .work/linux/serial-timeout
```

在有 AArch64 Linux 工具链时可以为 64 位 Arm 目标构建。使用与目标根文件系统兼容的工具链和 libc。

```sh
make -C examples/linux CC=aarch64-linux-gnu-gcc OUT=../../.work/linux-arm64
file .work/linux-arm64/serial-timeout
```

在板上复制到目标路径，确认二进制可执行，先按 [串口限时读取](serial-timeout.md) 观察有数据和超时两种情况。构建嵌入式镜像时，将这个程序当作普通用户态包集成。Buildroot 的外部树可以使用 `BR2_PACKAGE_...`、`TARGET_CC` 与目标安装目录，Yocto recipe 则从构建环境取得 `CC`，在 `do_install` 将产物放进 `${D}${bindir}`。安装路径与实际启动单元保持一致。

## 用一次服务执行记录一次观察

使用 systemd 的实验系统可以采用下面的一次性服务。部署前将用户和设备名换成实际值，并给专用用户赋予该串口设备的访问权限。根文件系统不含 systemd 时，使用目标系统的服务管理方式启动同一个命令。

<<< ../../examples/linux/serial-timeout/serial-check.service{ini}

安装服务到实验系统后，执行 `systemctl start serial-check.service`，再用 `journalctl -u serial-check.service` 查看本次输出。读取到字节返回 0，无数据返回 2，因此示例用 `SuccessExitStatus=2` 允许正常超时。访问权限或设备断开会走失败分支。

**预测题**　串口空闲，日志出现 `timeout`，服务状态为什么还能显示成功？

**改动题**　将设备路径改成一个不存在的测试路径，重新执行后检查日志和退出状态。

<details><summary>核对结果</summary>

先看程序退出码，再看服务如何解释它。普通超时返回 2，是配置允许的结果。打不开设备时返回 1，服务应记录失败和错误原因。

</details>

## 验证记录

已在 Alpine 3.22.8 通过仓库 Makefile 编译两个本地用户态示例。AArch64 交叉部署、Buildroot / Yocto 镜像集成及 systemd 服务运行待目标系统验证。
