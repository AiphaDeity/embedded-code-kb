#include <stdio.h>

int main(void)
{
    unsigned flags = 0x08u;
    flags |= 1u << 1;
    flags &= ~(1u << 3);
    printf("%02X %d\n", flags, (flags & (1u << 1)) != 0u);
    return 0;
}
