#include <stdio.h>

int main(void)
{
    int samples[] = {10, 20, 30, 40};
    int sum = 0;
    for (size_t i = 0; i < 4; ++i) {
        sum += samples[i];
    }
    printf("sum=%d mean=%d\n", sum, sum / 4);
    return 0;
}
