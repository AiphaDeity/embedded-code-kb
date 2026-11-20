#include <stdio.h>

enum Mode { MODE_IDLE, MODE_RUN, MODE_FAULT };

static const char *mode_name(enum Mode mode)
{
    switch (mode) {
    case MODE_IDLE:  return "idle";
    case MODE_RUN:   return "run";
    case MODE_FAULT: return "fault";
    default:         return "unknown";
    }
}

int main(void)
{
    puts(mode_name(MODE_RUN));
    return 0;
}
