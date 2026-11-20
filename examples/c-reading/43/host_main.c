#include "rx.h"
#include <stdio.h>

static void drain(RxQueue *queue, Decoder *decoder)
{
    RxByte byte;
    while (rx_pop(queue, &byte)) decode(decoder, byte);
}

int main(void)
{
    RxQueue queue = {0};
    Decoder decoder = {0};
    uint8_t input[] = {0, 0xA5, 0x2A, 0xD5, 0xA5, 1, 0};
    for (size_t i = 0; i < sizeof input; ++i) rx_push(&queue, input[i]);
    drain(&queue, &decoder);
    printf("good=%u bad=%u last=%u\n", decoder.good, decoder.bad, (unsigned)decoder.last);

    for (unsigned i = 0; i < RX_SLOTS - 1u; ++i) rx_push(&queue, 0);
    rx_push(&queue, 0xA5);
    rx_push(&queue, 0x11);  /* full, simulate a lost byte */
    drain(&queue, &decoder);
    rx_push(&queue, 0xA5);
    rx_push(&queue, 0x07);
    rx_push(&queue, 0xF8);
    drain(&queue, &decoder);
    printf("good=%u bad=%u last=%u peak=%u dropped=%u gaps=%u\n",
           decoder.good, decoder.bad, (unsigned)decoder.last,
           queue.peak, queue.dropped, decoder.gaps);
    return 0;
}
