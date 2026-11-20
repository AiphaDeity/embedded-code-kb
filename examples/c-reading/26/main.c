#include <stdio.h>
#include <stdbool.h>

typedef struct {
    bool stable;
    unsigned different;
} Button;

static bool update(Button *b, bool level)
{
    if (level == b->stable) {
        b->different = 0;
        return false;
    }
    if (++b->different < 3) {
        return false;
    }
    b->stable = level;
    b->different = 0;
    return true;
}

int main(void)
{
    Button b = {0};
    bool input[] = {1, 0, 1, 1, 1};
    for (size_t i = 0; i < 5; ++i) {
        if (update(&b, input[i])) {
            printf("%zu:%d\n", i + 1, b.stable);
        }
    }
    return 0;
}
