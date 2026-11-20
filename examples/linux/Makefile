CC ?= cc
CFLAGS ?= -O2
WARN := -std=c11 -Wall -Wextra -Werror
OUT ?= ../../.work/linux

.PHONY: all
all: $(OUT)/serial-timeout $(OUT)/poll-ipc

$(OUT):
	mkdir -p "$@"

$(OUT)/serial-timeout: serial-timeout/main.c | $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARN) $< $(LDFLAGS) -o "$@"

$(OUT)/poll-ipc: poll-ipc/main.c | $(OUT)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARN) $< $(LDFLAGS) -o "$@"
