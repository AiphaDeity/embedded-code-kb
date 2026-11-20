#include <stdio.h>
#include <stdatomic.h>
#include <stdbool.h>

typedef struct {
    atomic_bool full;
    unsigned value;
} Mailbox;

static bool publish(Mailbox *m, unsigned value)
{
    if (atomic_load_explicit(&m->full, memory_order_acquire)) return false;
    m->value = value;
    atomic_store_explicit(&m->full, true, memory_order_release);
    return true;
}

static bool take(Mailbox *m, unsigned *value)
{
    if (!atomic_load_explicit(&m->full, memory_order_acquire)) return false;
    *value = m->value;
    atomic_store_explicit(&m->full, false, memory_order_release);
    return true;
}

int main(void)
{
    Mailbox m;
    atomic_init(&m.full, false);
    unsigned value = 0;
    publish(&m, 7);
    if (!publish(&m, 8)) printf("busy ");
    if (take(&m, &value)) printf("%u ", value);
    if (publish(&m, 8)) puts("accepted");
    return 0;
}
