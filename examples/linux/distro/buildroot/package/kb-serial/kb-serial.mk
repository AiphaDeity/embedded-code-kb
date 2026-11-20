KB_SERIAL_VERSION = 1.0
KB_SERIAL_SITE = $(BR2_EXTERNAL_EMBEDDED_CODE_KB_PATH)/../../serial-timeout
KB_SERIAL_SITE_METHOD = local

# 1. Buildroot 的本地源复制完成后，用目标工具链编译已有串口示例。
define KB_SERIAL_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CPPFLAGS) $(TARGET_CFLAGS) -std=c11 \
		"$(@D)/main.c" $(TARGET_LDFLAGS) -o "$(@D)/kb-serial"
endef

# 2. 目标文件进入 TARGET_DIR，后续打包根文件系统时纳入 /usr/bin。
define KB_SERIAL_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 "$(@D)/kb-serial" "$(TARGET_DIR)/usr/bin/kb-serial"
endef

$(eval $(generic-package))
