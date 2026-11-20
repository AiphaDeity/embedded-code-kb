#include <stdio.h>
#include <stdbool.h>

typedef struct {
    unsigned slots[2];
    unsigned read, write, used, lost;
} Capture;

static void complete(Capture *c, unsigned sample)
{
    if (c->used == 2) {
        ++c->lost;
        return;
    }
    c->slots[c->write] = sample;
    c->write ^= 1u;
    ++c->used;
}

static bool consume(Capture *c, unsigned *out)
{
    if (c->used == 0) return false;
    *out = c->slots[c->read];
    c->read ^= 1u;
    --c->used;
    return true;
}

int main(void)
{
    Capture c = {0};
    unsigned value;
    complete(&c, 10);
    complete(&c, 20);
    complete(&c, 30);
    if (consume(&c, &value)) printf("%u ", value);
    complete(&c, 40);
    while (consume(&c, &value)) printf("%u ", value);
    printf("lost=%u\n", c.lost);
    return 0;
}
