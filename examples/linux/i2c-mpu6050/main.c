#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

enum {
    WHO_AM_I = 0x75,
    PWR_MGMT_1 = 0x6b,
    TEMP_OUT_H = 0x41,
    RESET = 0x80,
    SLEEP = 0x40,
    CYCLE = 0x20,
    TEMP_DISABLE = 0x08,
};

/* 完整传完 count 条才返回 0。失败返回 -1 并保留 errno。 */
static int transfer(int fd, struct i2c_msg *messages, unsigned int count)
{
    struct i2c_rdwr_ioctl_data request = {.msgs = messages, .nmsgs = count};
    int done = ioctl(fd, I2C_RDWR, &request);
    if (done == (int)count)
        return 0;
    if (done >= 0)
        errno = EIO;
    return -1;
}

/* length 单位为字节。失败时调用者丢弃 out 中的结果。 */
static int read_registers(int fd, uint16_t address, uint8_t reg,
                          uint8_t *out, uint16_t length)
{
    // 1. 一次 ioctl 提交写寄存器号和读数据两条消息，中间用重复起始衔接。
    struct i2c_msg messages[] = {
        {.addr = address, .len = 1, .buf = &reg},
        {.addr = address, .flags = I2C_M_RD, .len = length, .buf = out},
    };
    return transfer(fd, messages, 2);
}

static int wake_sensor(int fd, uint16_t address, uint8_t power)
{
    uint8_t change = SLEEP | CYCLE | TEMP_DISABLE;
    if ((power & change) == 0)
        return 0;

    // 1. 退出休眠和循环模式并打开测温。写入时清掉复位位，保留其他设置。
    uint8_t data[] = {PWR_MGMT_1, power & ~(RESET | change)};
    struct i2c_msg message = {.addr = address, .len = sizeof(data), .buf = data};
    if (transfer(fd, &message, 1) < 0)
        return -1;

    // 2. 给首次采样留出 100 ms。被信号打断时继续等待剩余时间。
    struct timespec remaining = {.tv_nsec = 100000000L};
    while (nanosleep(&remaining, &remaining) < 0) {
        if (errno != EINTR)
            return -1;
    }
    return 0;
}

/* 逐步检查适配器、设备和数据。任一步失败均向终端报告且不输出读数。 */
static int read_temperature(int fd, uint16_t address)
{
    // 1. 检查组合 I2C 支持，再让 i2c-dev 拒绝内核已占用的地址。
    unsigned long functions;
    if (ioctl(fd, I2C_FUNCS, &functions) < 0) {
        perror("I2C_FUNCS");
        return 1;
    }
    if ((functions & I2C_FUNC_I2C) == 0) {
        fputs("adapter does not support plain I2C\n", stderr);
        return 1;
    }
    if (ioctl(fd, I2C_SLAVE, (unsigned long)address) < 0) {
        perror("I2C_SLAVE");
        return 1;
    }

    // 2. 先读设备身份。没有应答或 ID 不对时退出，电源寄存器还没写入。
    uint8_t id;
    if (read_registers(fd, address, WHO_AM_I, &id, 1) < 0) {
        perror("read WHO_AM_I");
        return 1;
    }
    if (id != 0x68) {
        fprintf(stderr, "unexpected WHO_AM_I: 0x%02x\n", (unsigned)id);
        return 1;
    }

    // 3. 读当前电源设置，只在需要时唤醒。完成后才向温度寄存器发组合读。
    uint8_t power;
    if (read_registers(fd, address, PWR_MGMT_1, &power, 1) < 0 ||
        wake_sensor(fd, address, power) < 0) {
        perror("enable temperature sampling");
        return 1;
    }
    uint8_t data[2];
    if (read_registers(fd, address, TEMP_OUT_H, data, sizeof(data)) < 0) {
        perror("read TEMP_OUT");
        return 1;
    }

    // 4. 高字节在前。按有符号补码还原后，换算为芯片温度并输出。
    int raw = ((int)data[0] << 8) | data[1];
    if (raw >= 0x8000)
        raw -= 0x10000;
    printf("id=0x%02x temp_raw=%d temp_c=%.2f\n", (unsigned)id, raw,
           raw / 340.0 + 36.53);
    return 0;
}

int main(int argc, char **argv)
{
    // 1. 设备路径由使用者指定。地址使用对应 AD0 电平的 7 位值。
    if (argc < 2 || argc > 3 ||
        (argc == 3 && strcmp(argv[2], "0x68") && strcmp(argv[2], "0x69"))) {
        fprintf(stderr, "usage: %s /dev/i2c-N [0x68|0x69]\n", argv[0]);
        return 2;
    }
    uint16_t address = argc == 3 && !strcmp(argv[2], "0x69") ? 0x69 : 0x68;

    // 2. 持有 fd 完成一次采样。采样成功或失败都在这里关闭 fd。
    int fd = open(argv[1], O_RDWR | O_CLOEXEC);
    if (fd < 0) {
        perror(argv[1]);
        return 1;
    }
    int result = read_temperature(fd, address);
    close(fd);
    return result;
}
