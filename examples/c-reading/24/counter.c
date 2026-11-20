#include "counter.h"

void counter_add(Counter *counter, unsigned delta)
{
    unsigned room = 100u - counter->value;
    counter->value += delta > room ? room : delta;
}
