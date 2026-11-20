#include <stdio.h>
#include <stdint.h>

int main(void)
{
    uint8_t a = 0, b = 1;
    int difference = a - b;
    uint8_t wrapped = (uint8_t)difference;
    printf("%d %u\n", difference, (unsigned)wrapped);
    return 0;
}
