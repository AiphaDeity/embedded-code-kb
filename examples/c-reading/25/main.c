#include <stdio.h>
#include <stdint.h>
#include <inttypes.h>

int main(void)
{
    uint32_t start = UINT32_MAX - 3u;
    uint32_t now = 3u;
    uint32_t elapsed = (uint32_t)(now - start);
    printf("%" PRIu32 " %s\n", elapsed, elapsed >= 7u ? "due" : "wait");
    return 0;
}
