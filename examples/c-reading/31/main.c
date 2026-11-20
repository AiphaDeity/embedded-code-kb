#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

enum { SLOTS = 2 };
typedef struct {
    uint8_t data[SLOTS][16];
    bool used[SLOTS];
} Pool;

static int acquire(Pool *p)
{
    for (int i = 0; i < SLOTS; ++i) {
        if (!p->used[i]) {
            p->used[i] = true;
            return i;
        }
    }
    return -1;
}

static void release(Pool *p, int id)
{
    if (id >= 0 && id < SLOTS) p->used[id] = false;
}

int main(void)
{
    Pool p = {0};
    int a = acquire(&p);
    int b = acquire(&p);
    int c = acquire(&p);
    p.data[a][0] = 42;
    release(&p, a);
    printf("%d %d %d %d\n", a, b, c, acquire(&p));
    return 0;
}
