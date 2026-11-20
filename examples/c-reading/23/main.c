#include <stdio.h>

typedef void (*Visitor)(int sample, void *context);

static void visit(const int *a, size_t n, Visitor fn, void *context)
{
    for (size_t i = 0; i < n; ++i) {
        fn(a[i], context);
    }
}

static void accumulate(int sample, void *context)
{
    int *sum = context;
    *sum += sample;
}

int main(void)
{
    int a[] = {2, 4, 6};
    int sum = 0;
    visit(a, 3, accumulate, &sum);
    printf("%d\n", sum);
    return 0;
}
