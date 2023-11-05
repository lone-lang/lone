# SPDX-License-Identifier: AGPL-3.0-or-later

directories.build := build
directories.create := $(directories.build)

phony += directories
directories:
	mkdir -p $(directories.create)

ifdef TARGET
  ifndef UAPI
    $(error UAPI must be defined when cross compiling)
  endif

  TARGET.triple := $(TARGET)-unknown-linux-elf
  override CC := clang -target $(TARGET.triple)
else
  TARGET := $(shell uname -m)
endif

override ARCH := $(TARGET)
override ARCH.c := arch/$(ARCH).c

CFLAGS := -Wall -Wextra -Wpedantic -Os
override directories := $(if $(UAPI),-isystem $(UAPI))
override definitions := -D LONE_ARCH=$(ARCH) -D LONE_ARCH_SOURCE='"$(ARCH.c)"' -D LONE_NR_SOURCE='"NR.c"'
override essential_flags := $(definitions) -ffreestanding -nostdlib -Wl,-elone_start -static -fno-omit-frame-pointer -fshort-enums
override CC := $(strip $(CC) $(directories))

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
