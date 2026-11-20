#include <stdio.h>

static int decode12(unsigned raw)
{
    int value = (int)(raw & 0xFFFu);
    if (value >= 2048) {
        value -= 4096;
    }
    return value;
}

int main(void)
{
    printf("%d %d %d\n", decode12(0xFFF), decode12(0x800), decode12(0x7FF));
    return 0;
}
