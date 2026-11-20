#include <stdio.h>

int main(void)
{
    unsigned value = 507;
    do {
        printf("%u", value % 10u);
        value /= 10u;
    } while (value != 0u);
    putchar('\n');
    return 0;
}
