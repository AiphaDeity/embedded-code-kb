#include <stdio.h>
#include <stdint.h>

enum State { HEADER, LENGTH, PAYLOAD };
typedef struct {
    enum State state;
    unsigned used, expected;
    uint8_t data[4];
} Parser;

static void feed(Parser *p, uint8_t byte)
{
    switch (p->state) {
    case HEADER:
        if (byte == 0xA5) p->state = LENGTH;
        break;
    case LENGTH:
        p->used = 0;
        p->expected = byte;
        if (byte == 0) puts("0");
        p->state = byte > 0 && byte <= sizeof p->data ? PAYLOAD : HEADER;
        break;
    case PAYLOAD:
        p->data[p->used++] = byte;
        if (p->used == p->expected) {
            printf("%u", p->used);
            for (unsigned i = 0; i < p->used; ++i) {
                printf(":%02X", (unsigned)p->data[i]);
            }
            putchar('\n');
            p->state = HEADER;
        }
        break;
    }
}

int main(void)
{
    Parser parser = {0};
    uint8_t first[] = {0, 0xA5, 2, 0x11};
    uint8_t second[] = {0x22, 0xA5, 0};
    for (size_t i = 0; i < sizeof first; ++i) feed(&parser, first[i]);
    for (size_t i = 0; i < sizeof second; ++i) feed(&parser, second[i]);
    return 0;
}
