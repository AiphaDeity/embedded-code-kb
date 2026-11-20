#include <stdio.h>
#include <stdbool.h>

static bool contains(const int *a, size_t n, int key)
{
    if (n == 0) {
        return false;
    }
    size_t mid = n / 2;
    if (a[mid] == key) {
        return true;
    }
    if (key < a[mid]) {
        return contains(a, mid, key);
    }
    return contains(a + mid + 1, n - mid - 1, key);
}

int main(void)
{
    int table[] = {2, 5, 8, 13};
    puts(contains(table, 4, 13) ? "yes" : "no");
    return 0;
}
