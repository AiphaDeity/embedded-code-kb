#include <stdio.h>
#include <stdint.h>

static uint16_t read_be16(const uint8_t data[2])
{
    return (uint16_t)(((unsigned)data[0] << 8) | data[1]);
}

int main(void)
{
    uint8_t data[] = {0x12, 0x34};
    printf("%u\n", (unsigned)read_be16(data));
    return 0;
}
