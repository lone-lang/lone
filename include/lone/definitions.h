/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_DEFINITIONS_HEADER
#define LONE_DEFINITIONS_HEADER

#include <linux/auxvec.h>
#include <linux/elf.h>
#include <asm/bitsperlong.h>

/* Alias a handful of names that ordinarily come from glibc's
 * <assert.h> and <limits.h>. Both headers transitively pull
 * in <features.h> and define __GLIBC__ even under
 * -ffreestanding -nostdlib, which then causes kernel UAPI
 * headers like <linux/stat.h> to hide POSIX-shared macros
 * behind !defined(__GLIBC__). Provide compiler-intrinsic
 * equivalents instead and leave glibc out of the picture. */

#ifndef static_assert
#define static_assert _Static_assert
#endif

#ifndef CHAR_BIT
#define CHAR_BIT __CHAR_BIT__
#endif

#ifndef offsetof
#define offsetof __builtin_offsetof
#endif

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

#define LONE_MEMORY_SLAB_MIN        sizeof(void *)
#define LONE_MEMORY_SLAB_MAX        4096
#define LONE_MEMORY_SLAB_SIZE       65536
#define LONE_MEMORY_SLAB_CLASSES    (LONE_LOG2(LONE_MEMORY_SLAB_MAX) - LONE_LOG2(LONE_MEMORY_SLAB_MIN) + 1)

static_assert((LONE_MEMORY_SLAB_MIN & (LONE_MEMORY_SLAB_MIN - 1)) == 0, "LONE_MEMORY_SLAB_MIN must be a power of 2");
static_assert((LONE_MEMORY_SLAB_MAX & (LONE_MEMORY_SLAB_MAX - 1)) == 0, "LONE_MEMORY_SLAB_MAX must be a power of 2");
static_assert(LONE_MEMORY_SLAB_MAX >= 4096, "Size classes must cover at least 4096 bytes (minimum Linux page size)");
static_assert(LONE_MEMORY_SLAB_SIZE >= LONE_MEMORY_SLAB_MAX, "Slab size must be at least as large as the maximum size class");

#define LONE_MINIMUM_ALIGNMENT LONE_MEMORY_SLAB_MIN

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
