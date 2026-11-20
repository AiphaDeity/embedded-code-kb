#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

enum { CAP = 4 };
typedef struct {
    uint8_t data[CAP];
    unsigned read, write;
} Ring;

static bool push(Ring *r, uint8_t value)
{
    unsigned next = (r->write + 1u) % CAP;
    if (next == r->read) return false;
    r->data[r->write] = value;
    r->write = next;
    return true;
}

static bool pop(Ring *r, uint8_t *value)
{
    if (r->read == r->write) return false;
    *value = r->data[r->read];
    r->read = (r->read + 1u) % CAP;
    return true;
}

int main(void)
{
    Ring r = {0};
    uint8_t value;
    push(&r, 10);
    push(&r, 20);
    push(&r, 30);
    if (!push(&r, 40)) puts("full");
    pop(&r, &value);
    push(&r, 40);
    while (pop(&r, &value)) {
        printf("%u%c", (unsigned)value, r.read == r.write ? '\n' : ' ');
    }
    return 0;
}
