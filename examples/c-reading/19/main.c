#include <stdio.h>

typedef struct {
    unsigned count;
} Channel;

static void record(Channel *channel)
{
    ++channel->count;
}

int main(void)
{
    Channel a = {0}, b = {0};
    record(&a);
    record(&b);
    record(&a);
    printf("%u %u\n", a.count, b.count);
    return 0;
}
