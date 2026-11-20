#include "rx.h"

void rx_push(volatile RxQueue *q, uint8_t value)
{
    if (q->depth == RX_SLOTS) {
        ++q->dropped;
        q->gap_pending = true;
        return;
    }
    q->slots[q->write].value = value;
    q->slots[q->write].gap_before = q->gap_pending;
    q->gap_pending = false;
    q->write = (q->write + 1u) % RX_SLOTS;
    ++q->depth;
    if (q->depth > q->peak) q->peak = q->depth;
}

bool rx_pop(volatile RxQueue *q, RxByte *out)
{
    if (q->depth == 0) return false;
    *out = q->slots[q->read];
    q->read = (q->read + 1u) % RX_SLOTS;
    --q->depth;
    return true;
}

void decode(volatile Decoder *d, RxByte byte)
{
    if (byte.gap_before) {
        d->phase = WAIT_HEADER;
        ++d->gaps;
    }
    switch (d->phase) {
    case WAIT_HEADER:
        if (byte.value == 0xA5) d->phase = WAIT_DATA;
        break;
    case WAIT_DATA:
        d->pending = byte.value;
        d->phase = WAIT_CHECK;
        break;
    case WAIT_CHECK: {
        uint8_t check = (uint8_t)(d->pending ^ 0xFFu);
        if (byte.value == check) {
            d->last = d->pending;
            ++d->good;
        } else {
            ++d->bad;
        }
        d->phase = WAIT_HEADER;
        break;
    }
    }
}
