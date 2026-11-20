#include <stdio.h>
#include <stdint.h>

static uint8_t crc8(const uint8_t *data, size_t size)
{
    uint8_t crc = 0;
    for (size_t i = 0; i < size; ++i) {
        crc ^= data[i];
        for (unsigned bit = 0; bit < 8; ++bit) {
            unsigned polynomial = (crc & 0x80u) ? 0x07u : 0u;
            crc = (uint8_t)(((unsigned)crc << 1) ^ polynomial);
        }
    }
    return crc;
}

int main(void)
{
    uint8_t data[] = {0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39};
    printf("%02X\n", (unsigned)crc8(data, sizeof data));
    return 0;
}
