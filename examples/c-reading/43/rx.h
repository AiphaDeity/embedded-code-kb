#ifndef RX_PIPELINE_H
#define RX_PIPELINE_H

#include <stdbool.h>
#include <stdint.h>

enum { RX_SLOTS = 16 };
typedef struct {
    uint8_t value;
    bool gap_before;
} RxByte;

typedef struct {
    RxByte slots[RX_SLOTS];
    unsigned read, write, depth, peak, dropped;
    bool gap_pending;
} RxQueue;

typedef enum { WAIT_HEADER, WAIT_DATA, WAIT_CHECK } Phase;
typedef struct {
    Phase phase;
    uint8_t pending, last;
    unsigned good, bad, gaps;
} Decoder;

/* Queue operations require a single context or external exclusion. */
void rx_push(volatile RxQueue *q, uint8_t value);
bool rx_pop(volatile RxQueue *q, RxByte *out);
void decode(volatile Decoder *d, RxByte byte);

#endif
