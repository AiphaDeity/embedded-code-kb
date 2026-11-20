#include <stdio.h>

static const char *value_of(const char *p)
{
    while (*p != '\0' && *p != '=') {
        ++p;
    }
    if (*p == '=') {
        return p + 1;
    }
    return NULL;
}

int main(void)
{
    const char *value = value_of("rate=50");
    puts(value ? value : "missing");
    return 0;
}
