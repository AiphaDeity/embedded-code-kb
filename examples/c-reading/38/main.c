#include <stdio.h>
#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint8_t type;
    uint16_t seq;
} Header;

_Static_assert(offsetof(Header, type) == 0, "first member offset");

static void encode(uint8_t out[3], const Header *h)
{
    out[0] = h->type;
    out[1] = (uint8_t)(h->seq >> 8);
    out[2] = (uint8_t)h->seq;
}

int main(void)
{
    Header h = {.type = 0x2A, .seq = 0x1234};
    uint8_t wire[3];
    encode(wire, &h);
    printf("%02X %02X %02X\n", (unsigned)wire[0], (unsigned)wire[1], (unsigned)wire[2]);
    return 0;
}
