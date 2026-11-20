#include <stdio.h>

static const char *level(int celsius)
{
    if (celsius >= 40) {
        return "hot";
    }
    if (celsius >= 30) {
        return "warm";
    }
    return "normal";
}

int main(void)
{
    printf("%s %s %s\n", level(29), level(30), level(40));
    return 0;
}
