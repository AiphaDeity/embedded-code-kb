# 将同一份串口工具加入 Buildroot 和 Yocto

使用[串口限时读取](serial-timeout.md)的同一份 `main.c`。构建系统调用目标编译器编译它，并把 `kb-serial` 放进镜像的 `/usr/bin`。启动目标系统后，可以用该程序从串口读一批字节并打印十六进制。

## Buildroot 的 br2-external

`external.desc` 定义外部树名称。`Config.in` 让软件包出现在配置菜单。`external.mk` 把软件包的构建规则交给 Buildroot。包用 `local` 方法从本仓库取源码。

<a href="../../examples/linux/distro/buildroot/external.desc" download="external.desc">下载 external.desc</a>
· <a href="../../examples/linux/distro/buildroot/Config.in" download="Config.in">下载外部树 Config.in</a>
· <a href="../../examples/linux/distro/buildroot/external.mk" download="external.mk">下载 external.mk</a>
· <a href="../../examples/linux/distro/buildroot/package/kb-serial/Config.in" download="Config.in">下载包 Config.in</a>

<a href="../../examples/linux/distro/buildroot/package/kb-serial/kb-serial.mk" download="kb-serial.mk">下载 kb-serial.mk</a>

<<< ../../examples/linux/distro/buildroot/package/kb-serial/kb-serial.mk{makefile}

在 Linux 构建机上，把 `KB` 替换为本知识库的绝对路径，进入 Buildroot 源码目录。以下以 2025.02 分支自带的 x86-64 QEMU 配置为例。

```sh
KB=/absolute/path/embedded-code-kb
make BR2_EXTERNAL="$KB/examples/linux/distro/buildroot" qemu_x86_64_defconfig
make menuconfig
```

在 `External options` 中启用 `kb-serial`，保存后执行 `make kb-serial`。包安装步骤把可执行文件写到 `output/target/usr/bin/kb-serial`。执行 `make` 生成完整镜像后，在目标系统检查 `/usr/bin/kb-serial`。

## Yocto 的自定义层

Scarthgap（Yocto 5.0 系列）的层配置把 `kb-serial` 配方交给 BitBake。配方展开 `FILESEXTRAPATHS` 找到原串口源码，将程序安装到 `${D}${bindir}`，镜像按配方名选入软件包。练习库暂未声明源码许可，示例中的 `CLOSED` 是本地配方的构建标记。

<a href="../../examples/linux/distro/meta-embedded-code-kb/conf/layer.conf" download="layer.conf">下载 layer.conf</a>

<<< ../../examples/linux/distro/meta-embedded-code-kb/conf/layer.conf{text}

<a href="../../examples/linux/distro/meta-embedded-code-kb/recipes-support/kb-serial/kb-serial_1.0.bb" download="kb-serial_1.0.bb">下载 BitBake 配方</a>

<<< ../../examples/linux/distro/meta-embedded-code-kb/recipes-support/kb-serial/kb-serial_1.0.bb{text}

在配套版本的 Poky 构建环境中执行以下命令，使用自己的知识库绝对路径。

```sh
KB=/absolute/path/embedded-code-kb
. oe-init-build-env build-kb
bitbake-layers add-layer "$KB/examples/linux/distro/meta-embedded-code-kb"
bitbake-layers show-recipes kb-serial
bitbake kb-serial
```

把 `IMAGE_INSTALL:append = " kb-serial"` 添入这个实验构建目录的 `conf/local.conf` 后，再构建目标镜像，例如 `bitbake core-image-minimal`。镜像启动后确认工具存在。

## 运行和练习

板端准备一个 115200、8N1、可读写的串口。执行 `/usr/bin/kb-serial /dev/ttyUSB0`，在一秒等待窗口内从对端发送 `41 00 7E` 三个字节。串口设备名以目标系统为准。

**预测题**　`bitbake kb-serial` 完成后，之前已经生成的镜像会立刻包含这个程序吗？

<details><summary>核对预测</summary>

这一步生成软件包。把它加入镜像选包列表并重新构建镜像，启动该版本后才会看到它。

</details>

**改动题**　试着把安装文件名改为 `kb-read-serial`，同步更新两个包的目标路径。

<details><summary>核对改法</summary>

Buildroot 的安装目标和 Yocto 的 `do_install()` 改成新文件名。重新产出并部署镜像后，应能用新路径读取同一组串口数据。

</details>

## 验证记录

用 Buildroot 2025.02.12 加载外部树与 `qemu_x86_64_defconfig`，核对 `BR2_PACKAGE_KB_SERIAL=y` 和本地源码目录。在独立验证环境用 BitBake 2.8.0 解析层和配方，确认 `SRC_URI` 及源码路径。

在 Alpine 3.22.8 WSL 执行两套配方的编译、安装命令，生成 `/usr/bin/kb-serial`。两个产物均通过伪终端检查，覆盖超时、字节读取和退出时恢复串口设置。完整镜像的工具链构建、打包和目标系统启动待验收。

[Buildroot 外部树手册](https://buildroot.org/downloads/manual/manual.html#outside-br-custom) · [Yocto Scarthgap 编写配方手册](https://docs.yoctoproject.org/scarthgap/dev-manual/new-recipe.html)
