#include <stdio.h>

static unsigned count_high(const int *a, size_t n, int limit)
{
    unsigned count = 0;
    for (size_t i = 0; i < n; ++i) {
        if (a[i] >= limit) ++count;
    }
    return count;
}

int main(void)
{
    int a[] = {2, 3, 7, 1};
    printf("%u\n", count_high(a, 4, 3));
    return 0;
}
