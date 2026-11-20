#include <stdio.h>

unsigned gain = 3;
unsigned cache[8];
const unsigned table[] = {5, 7};

int main(void)
{
    unsigned total = cache[0] + gain + table[1];
    printf("%u\n", total);
    return 0;
}
