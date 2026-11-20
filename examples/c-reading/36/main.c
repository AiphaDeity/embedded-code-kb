#include <stdio.h>
#include <stdbool.h>

typedef bool (*ReadSensor)(void *context, int *value);

typedef struct {
    ReadSensor read;
    void *context;
    unsigned failures;
    bool valid;
    int last;
} Monitor;

static const char *poll_sensor(Monitor *m)
{
    int value;
    if (m->read(m->context, &value)) {
        m->last = value;
        m->valid = true;
        m->failures = 0;
        return "ok";
    }
    if (m->failures < 2) ++m->failures;
    return m->failures == 2 ? "fault" : "retry";
}

typedef struct {
    bool ok;
    int value;
} Reading;

typedef struct {
    const Reading *steps;
    size_t count, next;
} Script;

static bool scripted_read(void *context, int *value)
{
    Script *s = context;
    if (s->next == s->count) return false;
    Reading step = s->steps[s->next++];
    if (!step.ok) return false;
    *value = step.value;
    return true;
}

int main(void)
{
    Reading steps[] = {{true, 25}, {false, 0}, {false, 0}, {true, 27}};
    Script script = {steps, sizeof steps / sizeof steps[0], 0};
    Monitor m = {.read = scripted_read, .context = &script};
    for (size_t i = 0; i < script.count; ++i) {
        const char *status = poll_sensor(&m);
        if (m.valid) printf("%s %d\n", status, m.last);
        else printf("%s --\n", status);
    }
    return 0;
}
