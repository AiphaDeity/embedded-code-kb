#include <stdio.h>

static unsigned with_mode(unsigned word, unsigned mode)
{
    const unsigned mask = 3u << 4;
    return (word & ~mask) | ((mode & 3u) << 4);
}

int main(void)
{
    printf("%02X\n", with_mode(0xB5u, 2u));
    return 0;
}
