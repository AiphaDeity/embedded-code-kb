#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    const uint8_t *data;
    size_t size;
} View;

static bool parse(const uint8_t *p, size_t n, View *out)
{
    if (n < 2 || p[0] != 0xA5 || p[1] != n - 2) return false;
    out->data = p + 2;
    out->size = p[1];
    return true;
}

int main(void)
{
    uint8_t frame[] = {0xA5, 2, 0x11, 0x22};
    View view;
    if (parse(frame, sizeof frame, &view)) {
        printf("%zu %02X ", view.size, (unsigned)view.data[0]);
        frame[2] = 0x99;
        printf("%02X\n", (unsigned)view.data[0]);
    }
    return 0;
}
