#include <stdio.h>

static int clamp_duty(int duty)
{
    if (duty < 0) {
        return 0;
    }
    if (duty > 100) {
        return 100;
    }
    return duty;
}

int main(void)
{
    printf("%d %d %d\n", clamp_duty(-4), clamp_duty(35), clamp_duty(120));
    return 0;
}
