#include <stdio.h>

#define MAX2(a, b) ((a) > (b) ? (a) : (b))

static unsigned sample(unsigned *counter)
{
    return ++*counter;
}

static inline unsigned max2(unsigned a, unsigned b)
{
    return a > b ? a : b;
}

int main(void)
{
    unsigned reads = 0;
    unsigned macro_value = MAX2(sample(&reads), 0u);
    reads = 0;
    unsigned function_value = max2(sample(&reads), 0u);
    printf("%u %u\n", macro_value, function_value);
    return 0;
}
