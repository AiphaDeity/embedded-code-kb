#include <stdio.h>
#include <stdint.h>

enum { WINDOW = 3 };
typedef struct {
    uint16_t values[WINDOW];
    uint32_t sum;
    unsigned pos, used;
} Filter;

static uint16_t push(Filter *f, uint16_t sample)
{
    f->sum -= f->values[f->pos];
    f->values[f->pos] = sample;
    f->sum += sample;
    f->pos = (f->pos + 1u) % WINDOW;
    if (f->used < WINDOW) ++f->used;
    return (uint16_t)(f->sum / f->used);
}

int main(void)
{
    Filter f = {0};
    for (unsigned i = 1; i <= 4; ++i) {
        printf("%u%c", (unsigned)push(&f, (uint16_t)(10u * i)), i == 4 ? '\n' : ' ');
    }
    return 0;
}
