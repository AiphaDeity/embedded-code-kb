#include <stdio.h>

static unsigned twice(unsigned value)
{
    return value * 2u;
}

int main(void)
{
    unsigned reading = 17u;
    unsigned result = twice(reading);
    printf("%u\n", result);
    return 0;
}
