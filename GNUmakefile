# SPDX-License-Identifier: AGPL-3.0-or-later

ARCH ?= $(shell uname -m)
ARCH.c := arch/$(ARCH).c
CFLAGS := -Wall -Wextra -Wpedantic -Os -fwhole-program
override definitions := -D LONE_ARCH=$(ARCH) -D LONE_ARCH_SOURCE='"$(ARCH.c)"' -D LONE_NR_SOURCE='"NR.c"'
override essential_flags := $(definitions) -ffreestanding -nostartfiles -nostdlib -static -fno-omit-frame-pointer

lone : lone.c NR.c $(ARCH.c)
	$(CC) $(essential_flags) $(CFLAGS) -o $@ $<

phony += clean
clean:
	rm -f lone NR.list NR.c

phony += test
test: lone
	scripts/test.bash

NR.list: scripts/NR.filter
	$(CC) -E -dM -include linux/unistd.h - < /dev/null | scripts/NR.filter > $@

NR.c: NR.list scripts/NR.generate
	scripts/NR.generate < $< > $@

.PHONY: $(phony)
