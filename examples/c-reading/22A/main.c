#include <stdint.h>
#include <stdio.h>

static size_t encode(uint8_t *dst, size_t capacity, uint8_t value)
{
    if (dst == NULL || capacity < 3) {
        return 0;
    }
    dst[0] = 0xA5;
    dst[1] = value;
    dst[2] = (uint8_t)(value ^ 0xFFu);
    return 3;
}

int main(void)
{
    uint8_t frame[3];
    uint8_t small[2] = {0xCC, 0xCC};
    size_t n = encode(frame, sizeof frame, 0x2A);
    printf("%zu %02X %02X %02X\n", n, (unsigned)frame[0],
           (unsigned)frame[1], (unsigned)frame[2]);
    n = encode(small, sizeof small, 0x2A);
    printf("%zu %02X\n", n, (unsigned)small[0]);
    printf("%zu\n", encode(NULL, 0, 0x2A));
    return 0;
}
