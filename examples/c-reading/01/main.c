#include <stdio.h>

int main(void)
{
    unsigned mv = 3305;
    printf("%u.%03u V\n", mv / 1000u, mv % 1000u);
    return 0;
}
