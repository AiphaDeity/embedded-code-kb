# Linux 设备树匹配和只读 platform 驱动

## 示例

<a href="../../examples/linux/platform-reading/kb_reading.c" download="kb_reading.c">下载示例源码</a>

<<< ../../examples/linux/platform-reading/kb_reading.c

## 设备树片段

<<< ../../examples/linux/platform-reading/reading-overlay.dts{text}

## 执行和验证

设备树给实验节点设置 `demo-value = <42>`。驱动通过 compatible 匹配，在 `probe()` 读取属性并创建只读 `value`。

需要一台启用设备树的 Linux 实验机、与目标系统匹配的内核构建目录和设备树编译器。使用目标板支持的 overlay 加载方法。仓库中的 Makefile 可编译模块和 overlay：

```sh
cd examples/linux/platform-reading
make KDIR=/lib/modules/$(uname -r)/build
```

在已登记 overlay 的实验系统加载 `kb_reading.ko`，核对内核日志，然后从 `/sys/bus/platform/devices/` 下定位对应设备的 `value`。预期读出 `42`。交叉编译时将 `KDIR` 指向目标内核构建树，并传入内核支持的 `ARCH` 与 `CROSS_COMPILE`。板子使用的 overlay 安装位置和启动配置以该系统文档为准。

**预测题**　如果 compatible 不匹配，会进入 probe 吗？

**改动题**　把 overlay 的值从 42 改为 99，按目标板流程重新应用后查看 sysfs。

<details><summary>核对改法</summary>

compatible 不匹配时，这个驱动不会进入 probe。改动属性后重新绑定成功，`value` 应显示 99。缺失属性时 probe 以错误返回并留下日志。

</details>

[Linux platform driver 文档](https://docs.kernel.org/driver-api/driver-model/platform.html)

## 验证记录

已在 Alpine 3.22.8 使用 `dtc` 编译 overlay，并用 `fdtget` 读回 `demo-value=42`。使用 GCC 14.2.0 和 Alpine x86_64 `6.12.118-0-virt` 的内核构建目录编译链接出 `kb_reading.ko`，并用 `readelf` 核对 OF 匹配别名和内核版本标记。目标板仍需使用其内核构建目录重新编译模块，并验证 overlay 加载、驱动绑定和 sysfs 实际读值。
