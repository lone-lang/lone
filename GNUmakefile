# SPDX-License-Identifier: AGPL-3.0-or-later

ifdef TARGET
  ifndef UAPI
    $(error UAPI must be defined when cross compiling)
  endif

  TARGET.triple := $(TARGET)-unknown-linux-elf
  override CC := clang -target $(TARGET.triple)
else
  TARGET := $(shell uname -m)
endif

ARCH := $(TARGET)
ARCH.c := arch/$(ARCH).c

directories.include := include
directories.build := build/$(ARCH)
directories.build.prerequisites := $(directories.build)/prerequisites
directories.create := $(directories.build) $(directories.build.prerequisites)

phony += directories
directories:
	mkdir -p $(directories.create)

add_prefix_and_suffix = $(addprefix $(1),$(addsuffix $(2),$(3)))
to_prerequisites = $(call add_prefix_and_suffix,$(directories.build.prerequisites)/,.d,$(basename $(1)))

flags.lone := -ffreestanding -nostdlib -Wl,-elone_start -static -fno-omit-frame-pointer -fshort-enums
flags.definitions := -D LONE_ARCH=$(ARCH) -D LONE_ARCH_SOURCE='"$(ARCH.c)"' -D LONE_NR_SOURCE='"NR.c"'
flags.include_directories := -I $(directories.include)
flags.system_include_directories = $(if $(UAPI),-isystem $(UAPI))
flags.prerequisites_generation = -MMD -MF $(call to_prerequisites,$(<))
flags.all = $(flags.system_include_directories) $(flags.include_directories) $(flags.prerequisites_generation) $(flags.definitions) $(flags.lone)

CFLAGS := -Wall -Wextra -Wpedantic -Os

targets.lone := $(directories.build)/lone

$(targets.lone): lone.c NR.c $(ARCH.c) | directories
	$(strip $(CC) $(flags.all) $(CFLAGS) -o $@ $<)

phony += lone
lone: $(targets.lone)

phony += clean
clean:
	rm -f lone NR.list NR.c
	rm -rf $(directories.create)

phony += test
test: lone
	scripts/test.bash

NR.list: scripts/NR.filter
	$(CC) -E -dM -include linux/unistd.h - < /dev/null | scripts/NR.filter > $@

NR.c: NR.list scripts/NR.generate
	scripts/NR.generate < $< > $@

.PHONY: $(phony)
.DEFAULT_GOAL := lone
