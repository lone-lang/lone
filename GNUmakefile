# SPDX-License-Identifier: AGPL-3.0-or-later

ARCH ?= $(shell uname -m)
ARCH.c := arch/$(ARCH).c
CFLAGS := -Wall -Wextra -Wpedantic -Os
override essential_flags := -ffreestanding -nostartfiles -nostdlib -static -D LONE_ARCH=$(ARCH) -D LONE_ARCH_SOURCE='"$(ARCH.c)"'

lone : lone.c $(ARCH.c)
	$(CC) $(essential_flags) $(CFLAGS) -o $@ $<

phony += clean
clean:
	rm -f lone

phony += test
test: lone
	./test.bash

NR.list: scripts/NR.filter
	$(CC) -E -dM -include linux/unistd.h - < /dev/null | scripts/NR.filter > $@

.PHONY: $(phony)
