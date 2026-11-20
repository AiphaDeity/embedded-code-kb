#include <stdio.h>
#include <stdbool.h>

static bool minmax(const int *a, size_t n, int *lo, int *hi)
{
    if (n == 0) {
        return false;
    }
    *lo = *hi = a[0];
    for (size_t i = 1; i < n; ++i) {
        if (a[i] < *lo) *lo = a[i];
        if (a[i] > *hi) *hi = a[i];
    }
    return true;
}

int main(void)
{
    int a[] = {8, 3, 12};
    int lo, hi;
    if (minmax(a, 3, &lo, &hi)) {
        printf("%d %d\n", lo, hi);
    }
    return 0;
}
