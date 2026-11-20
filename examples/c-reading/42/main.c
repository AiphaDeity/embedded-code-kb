#include <stdio.h>
#include <stdint.h>

enum Phase { SYNC, DATA, CHECK };
typedef struct {
    enum Phase phase;
    uint8_t pending, last;
    unsigned good, bad;
} Decoder;

static void decode(Decoder *d, const uint8_t *bytes, size_t n)
{
    for (size_t i = 0; i < n; ++i) {
        uint8_t byte = bytes[i];
        switch (d->phase) {
        case SYNC:
            if (byte == 0xA5) d->phase = DATA;
            break;
        case DATA:
            d->pending = byte;
            d->phase = CHECK;
            break;
        case CHECK: {
            uint8_t expected = (uint8_t)(d->pending ^ 0xFFu);
            if (byte == expected) {
                ++d->good;
                d->last = d->pending;
            } else {
                ++d->bad;
            }
            d->phase = SYNC;
            break;
        }
        }
    }
}

int main(void)
{
    Decoder decoder = {0};
    uint8_t a[] = {0x00, 0xA5, 0x10};
    uint8_t b[] = {0xEF, 0xA5, 0x11, 0x00, 0xA5};
    uint8_t c[] = {0x2A, 0xD5};
    decode(&decoder, a, sizeof a);
    decode(&decoder, b, sizeof b);
    decode(&decoder, c, sizeof c);
    printf("good=%u bad=%u last=%u\n", decoder.good, decoder.bad, (unsigned)decoder.last);
    return 0;
}
