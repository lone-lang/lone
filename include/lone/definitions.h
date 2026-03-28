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
	#define LONE_DECIMAL_DIGITS_PER_LONG 20
#elif __BITS_PER_LONG == 32
	#define LONE_DECIMAL_DIGITS_PER_LONG 10
#else
	#error "Unsupported architecture"
#endif

/* Compile-time log2 for power-of-2 constants.
 * Checks the lowest set bit position. */
#define LONE_LOG2(n) (             \
	((n) & 0x00001) ?  0 :     \
	((n) & 0x00002) ?  1 :     \
	((n) & 0x00004) ?  2 :     \
	((n) & 0x00008) ?  3 :     \
	((n) & 0x00010) ?  4 :     \
	((n) & 0x00020) ?  5 :     \
	((n) & 0x00040) ?  6 :     \
	((n) & 0x00080) ?  7 :     \
	((n) & 0x00100) ?  8 :     \
	((n) & 0x00200) ?  9 :     \
	((n) & 0x00400) ? 10 :     \
	((n) & 0x00800) ? 11 :     \
	((n) & 0x01000) ? 12 :     \
	((n) & 0x02000) ? 13 :     \
	((n) & 0x04000) ? 14 :     \
	((n) & 0x08000) ? 15 :     \
	((n) & 0x10000) ? 16 : -1  \
)

#ifndef LONE_ALIGNMENT
	#define LONE_ALIGNMENT 16
#endif

#ifndef LONE_MEMORY_SEGMENT_MINIMUM_SIZE
	#define LONE_MEMORY_SEGMENT_MINIMUM_SIZE (1024 * 1024)
#endif

#ifndef PT_LONE
//      PT_LONE   l o n e
#define PT_LONE 0x6c6f6e65
#endif

#if PT_LONE < PT_LOOS || PT_LONE > PT_HIOS
	#warning "PT_LONE outside reserved operating system specific range"
#endif

#define LONE_SIZE_OF_MEMBER(type, member) sizeof(((type) { 0 }).member)

#define LONE_WARN_UNUSED_RESULT __attribute__((warn_unused_result))

#endif /* LONE_DEFINITIONS_HEADER */
