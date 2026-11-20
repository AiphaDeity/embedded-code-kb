#include <stdio.h>

enum Kind { BUTTON, TEMPERATURE };
typedef struct {
    enum Kind kind;
    union {
        unsigned button;
        int celsius;
    } data;
} Event;

static void show(Event e)
{
    switch (e.kind) {
    case BUTTON:      printf("key %u\n", e.data.button); break;
    case TEMPERATURE: printf("%d C\n", e.data.celsius); break;
    }
}

int main(void)
{
    Event e = {.kind = TEMPERATURE, .data.celsius = -5};
    show(e);
    return 0;
}
