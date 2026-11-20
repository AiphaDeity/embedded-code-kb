#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static uint16_t *copy_samples(const uint16_t *src, size_t n)
{
    if (n == 0 || n > SIZE_MAX / sizeof *src) {
        return NULL;
    }
    uint16_t *copy = malloc(n * sizeof *copy);
    if (copy != NULL) {
        memcpy(copy, src, n * sizeof *copy);
    }
    return copy;
}

int main(void)
{
    uint16_t source[] = {10, 20};
    uint16_t *copy = copy_samples(source, 2);
    if (copy == NULL) {
        return 1;
    }
    source[1] = 99;
    printf("%u\n", (unsigned)copy[1]);
    free(copy);
    return 0;
}
