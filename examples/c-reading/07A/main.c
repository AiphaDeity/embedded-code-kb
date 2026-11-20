#include <stddef.h>
#include <stdio.h>

static void show(const int p[], size_t count)
{
    printf("%d %d\n", p[1], *(p + 1));
    printf("count=%zu pointer=%d\n", count, sizeof p == sizeof(const int *));
}

int main(void)
{
    int a[] = {12, 24, 36, 48};
    show(a, sizeof a / sizeof a[0]);
    return 0;
}
