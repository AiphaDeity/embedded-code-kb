#include <assert.h>
#include <errno.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned int stage;
static unsigned long address;

int __wrap_ioctl(int fd, unsigned long command, ...)
{
    assert(fd >= 0);
    const char *mode = getenv("MOCK_I2C");
    assert(mode != NULL);
    va_list args;
    va_start(args, command);

    if (command == I2C_FUNCS) {
        assert(stage++ == 0);
        *va_arg(args, unsigned long *) =
            strcmp(mode, "no-i2c") ? I2C_FUNC_I2C : I2C_FUNC_SMBUS_BYTE;
        va_end(args);
        return 0;
    }
    if (command == I2C_SLAVE) {
        assert(stage++ == 1);
        address = va_arg(args, unsigned long);
        assert(address == (!strcmp(mode, "normal") ? 0x69 : 0x68));
        va_end(args);
        if (strcmp(mode, "busy") == 0) {
            errno = EBUSY;
            return -1;
        }
        return 0;
    }

    assert(command == I2C_RDWR);
    struct i2c_rdwr_ioctl_data *request =
        va_arg(args, struct i2c_rdwr_ioctl_data *);
    va_end(args);
    struct i2c_msg *msg = request->msgs;
    assert(msg[0].addr == address && msg[0].flags == 0);

    switch (stage++) {
    case 2:
        assert(request->nmsgs == 2 && msg[0].buf[0] == 0x75);
        if (strcmp(mode, "short") == 0)
            return 1;
        msg[1].buf[0] = strcmp(mode, "wrong-id") ? 0x68 : 0x00;
        break;
    case 3:
        assert(request->nmsgs == 2 && msg[0].buf[0] == 0x6b);
        msg[1].buf[0] = 0x69;
        break;
    case 4:
        assert(request->nmsgs == 1 && msg[0].len == 2);
        assert(msg[0].buf[0] == 0x6b && msg[0].buf[1] == 0x01);
        return 1;
    case 5:
        assert(request->nmsgs == 2 && msg[0].buf[0] == 0x41);
        // FF AC 是有符号原始值 -84。
        msg[1].buf[0] = 0xff;
        msg[1].buf[1] = 0xac;
        break;
    default:
        abort();
    }

    assert(msg[0].len == 1 && msg[1].addr == address);
    assert(msg[1].flags == I2C_M_RD && msg[1].len == (stage == 6 ? 2 : 1));
    return 2;
}
