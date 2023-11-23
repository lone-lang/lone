/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_DEFINITIONS_HEADER
#define LONE_DEFINITIONS_HEADER

#include <linux/auxvec.h>
#include <linux/elf.h>
#include <asm/bitsperlong.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │                                      bits = 32    |    bits = 64       │
   │    digits = ceil(bits * log10(2)) =  10           |    20              │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#if __BITS_PER_LONG == 64
	#define DECIMAL_DIGITS_PER_LONG 20
#elif __BITS_PER_LONG == 32
	#define DECIMAL_DIGITS_PER_LONG 10
#else
	#error "Unsupported architecture"
#endif

#ifndef LONE_BUFFER_SIZE
	#define LONE_BUFFER_SIZE 4096
#endif

#ifndef LONE_MEMORY_SIZE
	#define LONE_MEMORY_SIZE (1024 * 1024)
#endif

#ifndef LONE_ALIGNMENT
	#define LONE_ALIGNMENT 16
#endif

#define LONE_PRIMITIVE(name)                  \
struct lone_value *lone_primitive_ ## name    \
(                                             \
	struct lone_lisp *lone,               \
	struct lone_value *module,            \
	struct lone_value *environment,       \
	struct lone_value *arguments,         \
	struct lone_value *closure            \
)

#ifndef PT_LONE
//      PT_LONE   l o n e
#define PT_LONE 0x6c6f6e65
#endif

#if PT_LONE < PT_LOOS || PT_LONE > PT_HIOS
	#warning "PT_LONE outside reserved operating system specific range"
#endif

#endif /* LONE_DEFINITIONS_HEADER */
