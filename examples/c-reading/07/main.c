#include <stdio.h>

static size_t first_high(const int *a, size_t n, int limit)
{
    for (size_t i = 0; i < n; ++i) {
        if (a[i] >= limit) {
            return i;
        }
    }
    return n;
}

int main(void)
{
    int a[] = {18, 20, 25};
    size_t n = sizeof a / sizeof a[0];
    printf("%zu\n", first_high(a, n, 20));
    return 0;
}
