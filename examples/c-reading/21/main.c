#include <stdio.h>

static unsigned next_id(void)
{
    static unsigned count;
    return ++count;
}

int main(void)
{
    unsigned first = next_id();
    unsigned second = next_id();
    printf("%u %u\n", first, second);
    return 0;
}
