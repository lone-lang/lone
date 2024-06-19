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

#ifndef LONE_ALIGNMENT
	#define LONE_ALIGNMENT 16
#endif

#ifndef PT_LONE
//      PT_LONE   l o n e
#define PT_LONE 0x6c6f6e65
#endif

#if PT_LONE < PT_LOOS || PT_LONE > PT_HIOS
	#warning "PT_LONE outside reserved operating system specific range"
#endif

#define LONE_SIZE_OF_MEMBER(type, member) sizeof(((type) { 0 }).member)

#endif /* LONE_DEFINITIONS_HEADER */
