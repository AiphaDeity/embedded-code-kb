#include <stdio.h>

int main(void)
{
    int samples[2][3] = {{1, 2, 3}, {4, 5, 6}};
    for (size_t channel = 0; channel < 2; ++channel) {
        int sum = 0;
        for (size_t tick = 0; tick < 3; ++tick) {
            sum += samples[channel][tick];
        }
        printf("%d\n", sum);
    }
    return 0;
}
