#include <stdio.h>

typedef struct {
    volatile unsigned status;
} DeviceModel;

/* 模拟外设收到一次写操作后的内部动作。 */
static void bus_write(DeviceModel *dev, unsigned written)
{
    dev->status &= ~written;
}

static void acknowledge_rx(DeviceModel *dev)
{
    bus_write(dev, 1u << 0);
}

int main(void)
{
    DeviceModel dev = {.status = 0x05u};
    acknowledge_rx(&dev);
    printf("%02X\n", dev.status);
    return 0;
}
