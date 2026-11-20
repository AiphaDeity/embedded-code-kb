#include <stdio.h>

int main(void)
{
    char text[4];
    const char *message = "ERR42";
    int needed = snprintf(text, sizeof text, "%s", message);
    if (needed >= 0 && (size_t)needed >= sizeof text) {
        printf("%s %d truncated\n", text, needed);
    }
    return 0;
}
