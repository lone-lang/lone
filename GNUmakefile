# SPDX-License-Identifier: AGPL-3.0-or-later

MAKEFLAGS += --no-builtin-variables --no-builtin-rules

nil :=
space := $(nil) $(nil)
$(space) := $(space)

variables.log = $(foreach variable,$(1),$(if $($(variable)),$(info $(variable) $($(variable)))))
newline = $(info $(nil))

CC ?= cc
CC := $(CC)

LD ?= ld
LD := $(LD)

CFLAGS ?= -Wall -Wextra -Wpedantic -Wno-unused-function -Wno-unused-parameter -Wno-unknown-attributes
CFLAGS := $(CFLAGS)

LDFLAGS ?=
LDFLAGS := $(LDFLAGS)

ifdef TARGET
  ifndef UAPI
    $(error UAPI must be defined when cross compiling)
  endif

  TARGET.triple := $(TARGET)-unknown-linux-elf
  override CC := clang -target $(TARGET.triple)
else
  TARGET := $(shell uname -m)
endif

source_to_object = $(patsubst $(directories.source)/%.c,$(directories.build.objects)/%.o,$(1))
source_to_prerequisite = $(patsubst $(directories.source)/%.c,$(directories.build.prerequisites)/%.d,$(1))
source_to_tool = $(patsubst $(directories.source.tools)/%.c,$(directories.build.tools)/%,$(1))
source_to_test = $(patsubst $(directories.source.tests)/%.c,$(directories.build.tests)/%,$(1))

ARCH := $(TARGET)

directories.build := build/$(ARCH)
directories.build.tools := $(directories.build)/tools
directories.build.tests := $(directories.build)/tests
directories.build.objects := $(directories.build)/objects
directories.build.objects.tools := $(directories.build.objects)/tools
directories.build.objects.tests := $(directories.build.objects)/tests
directories.build.prerequisites := $(directories.build)/prerequisites
directories.build.include := $(directories.build)/include
directories.create :=

directories.include := include architecture/$(ARCH)/include $(directories.build.include)
directories.source := source
directories.source.lone := $(directories.source)/lone
directories.source.tools := $(directories.source)/tools
directories.source.tests := $(directories.source)/tests
directories.test := test

files.sources.all := $(shell find $(directories.source) -type f)
files.sources.lone := $(filter $(directories.source.lone)/%,$(files.sources.all))
files.sources.tools := $(filter $(directories.source.tools)/%,$(files.sources.all))
files.sources.tests := $(filter $(directories.source.tests)/%,$(files.sources.all))

targets.phony :=
targets.NR.list := $(directories.build)/NR.list
targets.NR.c := $(directories.build.include)/lone/NR.c
targets.NR := $(targets.NR.list) $(targets.NR.c)
targets.objects.lone := $(call source_to_object,$(files.sources.lone))
targets.objects.lone.entry_point := $(directories.build.objects)/lone.o
targets.objects.tools := $(call source_to_object,$(files.sources.tools))
targets.objects.tests := $(call source_to_object,$(files.sources.tests))
targets.objects.all := $(targets.objects.lone) $(targets.objects.tools) $(targets.objects.tests)
targets.lone := $(directories.build)/lone
targets.prerequisites := $(call source_to_prerequisite,$(files.sources.all))
targets.tools := $(call source_to_tool,$(files.sources.tools))
targets.tests := $(call source_to_test,$(files.sources.tests))
targets.all := $(targets.lone) $(targets.tools) $(targets.tests) $(targets.objects.all) $(targets.NR) $(targets.prerequisites)

directories.create += $(dir $(targets.all))

export PATH := $(directories.build):$(directories.build.tools):$(directories.build.tests):$(PATH)

flags.whole_program.gcc := -fwhole-program
flags.whole_program := -fvisibility=hidden $(flags.whole_program.$(CC))

ifeq ($(LD),ld)
  # ld is already the default, don't override
  # gcc does not support ld as an argument either
  # not overriding in this case prevents errors
  flags.use_ld :=
else
  flags.use_ld := -fuse-ld=$(LD)
endif

ifdef LTO
  flags.lto := -flto
else
  flags.lto :=
endif

flags.definitions := -D LONE_ARCH=$(ARCH)
flags.include_directories := $(foreach directory,$(directories.include),-I $(directory))
flags.system_include_directories := $(if $(UAPI),-isystem $(UAPI))
flags.prerequisites_generation = -MMD -MF $(call source_to_prerequisite,$(<))
flags.common := -static -ffreestanding -nostdlib -fno-omit-frame-pointer -fshort-enums $(flags.lto)
flags.object = $(flags.system_include_directories) $(flags.include_directories) $(flags.prerequisites_generation) $(flags.definitions) $(flags.common)
flags.executable := $(flags.common) $(flags.whole_program) $(flags.use_ld) -Wl,-elone_start

# Disable strict aliasing and assume two's complement integers
# even if CFLAGS contains options that affect this such as -O3
CFLAGS.overrides := -fno-strict-aliasing -fwrapv -fno-stack-protector
CFLAGS.with_overrides := $(CFLAGS) $(CFLAGS.overrides)

$(directories.build.objects)/%.o: $(directories.source)/%.c | directories
	$(strip $(CC) $(flags.object) $(CFLAGS.with_overrides) -o $@ -c $<)

$(targets.lone): $(targets.objects.lone.entry_point) $(targets.objects.lone) | directories
	$(strip $(CC) $(flags.executable) $(CFLAGS.with_overrides) $(LDFLAGS) -o $@ $^)

$(directories.build.tools)/%: $(directories.build.objects.tools)/%.o $(targets.objects.lone) | directories
	$(strip $(CC) $(flags.executable) $(CFLAGS.with_overrides) $(LDFLAGS) -o $@ $^)

$(directories.build.tests)/%: $(directories.build.objects.tests)/%.o $(targets.objects.lone) | directories
	$(strip $(CC) $(flags.executable) $(CFLAGS.with_overrides) $(LDFLAGS) -o $@ $^)

$(call source_to_object,source/lone/modules/intrinsic/linux.c): $(targets.NR.c)

$(targets.NR.c): $(targets.NR.list) scripts/NR.generate
	scripts/NR.generate < $< > $@

$(targets.NR.list): scripts/NR.filter
	$(CC) -E -dM -include linux/unistd.h - < /dev/null | scripts/NR.filter > $@

targets.phony += lone
lone: $(targets.lone)

targets.phony += tools
tools: $(targets.tools)

targets.phony += clean
clean:
	rm -rf $(directories.build)

targets.phony += tests
tests: $(targets.tests)

targets.phony += test
test: tests lone tools
	scripts/test.bash $(directories.test)

targets.phony += directories
directories:
	mkdir -p $(sort $(directories.create))

.PHONY: $(targets.phony)
.SECONDARY: $(targets.objects.all)
.DEFAULT_GOAL := lone

sinclude $(targets.prerequisites)

$(call variables.log,TARGET TARGET.triple UAPI CC LD CFLAGS LDFLAGS LTO)
$(call newline)
