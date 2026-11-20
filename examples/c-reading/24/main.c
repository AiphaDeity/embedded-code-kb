#include <stdio.h>
#include "counter.h"

int main(void)
{
    Counter counter = {0};
    counter_add(&counter, 60);
    printf("%u ", counter.value);
    counter_add(&counter, 60);
    printf("%u\n", counter.value);
    return 0;
}
