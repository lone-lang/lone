# SPDX-License-Identifier: AGPL-3.0-or-later

ARCH ?= $(shell uname -m)
CFLAGS := -Wall -Wextra -Wpedantic -Os
override essential_flags := -ffreestanding -nostartfiles -nostdlib -static -include arch/$(ARCH).c

lone : lone.c arch/$(ARCH).c
	$(CC) $(essential_flags) $(CFLAGS) -o $@ $<

phony += clean
clean:
	rm -f lone

.PHONY: $(phony)
