#include <stdio.h>
#include <limits.h>
#include <stdbool.h>

static bool checked_add(int a, int b, int *out)
{
    if (b > 0 && a > INT_MAX - b) return false;
    if (b < 0 && a < INT_MIN - b) return false;
    *out = a + b;
    return true;
}

int main(void)
{
    int out = 11;
    if (!checked_add(INT_MAX, 1, &out)) printf("overflow %d\n", out);
    if (checked_add(5, -2, &out)) printf("%d\n", out);
    return 0;
}
