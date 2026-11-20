#include <stdio.h>

enum { CAPACITY = 3 };
typedef struct {
    unsigned depth, peak, dropped;
} QueueStats;

static void arrive(QueueStats *s)
{
    if (s->depth == CAPACITY) {
        ++s->dropped;
        return;
    }
    ++s->depth;
    if (s->depth > s->peak) s->peak = s->depth;
}

static void depart(QueueStats *s)
{
    if (s->depth != 0) --s->depth;
}

int main(void)
{
    QueueStats stats = {0};
    for (unsigned i = 0; i < 4; ++i) arrive(&stats);
    depart(&stats);
    depart(&stats);
    printf("depth=%u peak=%u dropped=%u\n", stats.depth, stats.peak, stats.dropped);
    return 0;
}
