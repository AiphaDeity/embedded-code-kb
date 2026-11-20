#ifndef COUNTER_H
#define COUNTER_H

typedef struct {
    unsigned value;  /* 合法范围 0..100 */
} Counter;

void counter_add(Counter *counter, unsigned delta);

#endif
