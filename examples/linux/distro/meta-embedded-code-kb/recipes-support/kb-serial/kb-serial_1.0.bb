SUMMARY = "Serial port reader with a bounded wait"
LICENSE = "CLOSED"

# 1. 将仓库已有的串口源码交给 BitBake 的本地文件获取任务。
FILESEXTRAPATHS:prepend := "${THISDIR}/../../../../serial-timeout:"
SRC_URI = "file://main.c;subdir=kb-serial"
S = "${WORKDIR}/kb-serial"
B = "${WORKDIR}/build"

# 2. 使用 Yocto 提供的目标编译器与 flags，产物写入独立构建目录。
do_compile() {
    ${CC} ${CPPFLAGS} ${CFLAGS} -std=c11 "${S}/main.c" ${LDFLAGS} -o "${B}/kb-serial"
}

# 3. 装入当前配方的安装根目录。后续打包任务从这里提取 kb-serial。
do_install() {
    install -d "${D}${bindir}"
    install -m 0755 "${B}/kb-serial" "${D}${bindir}/kb-serial"
}
