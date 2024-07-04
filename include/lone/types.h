/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_TYPES_HEADER
#define LONE_TYPES_HEADER

#include <stdbool.h>
#include <stdint.h>
#include <stdarg.h>

#include <linux/types.h>
#include <linux/elf.h>

#include <lone/definitions.h>

typedef __kernel_size_t size_t;
typedef __kernel_ssize_t ssize_t;
typedef __kernel_off_t off_t;

typedef __kernel_size_t lone_size;

typedef __u8  lone_u8;
typedef __s8  lone_s8;
typedef __u16 lone_u16;
typedef __s16 lone_s16;
typedef __u32 lone_u32;
typedef __s32 lone_s32;
typedef __u64 lone_u64;
typedef __s64 lone_s64;

#if __BITS_PER_LONG == 64
typedef Elf64_Ehdr lone_elf_header;
typedef Elf64_Phdr lone_elf_segment;
#elif __BITS_PER_LONG == 32
typedef Elf32_Ehdr lone_elf_header;
typedef Elf32_Phdr lone_elf_segment;
#else
	#error "Unsupported architecture"
#endif

struct lone_elf_segments {
	size_t entry_size;
	size_t entry_count;
	lone_elf_segment *segments;
};

struct lone_auxiliary_value {
	union {
		void *pointer;
		char *c_string;
		unsigned char *bytes;
		long signed_integer;
		unsigned long unsigned_integer;
	} as;
};

struct lone_auxiliary_vector {
	unsigned long type;
	struct lone_auxiliary_value value;
};

struct lone_bytes {
	size_t count;              /* size of memory block in bytes */
	unsigned char *pointer;    /* address of memory block */
};

#define LONE_BYTES_FROM_LITERAL(c_string_literal)                      \
	((struct lone_bytes) {                                         \
		.count = sizeof(c_string_literal) - 1,                 \
		.pointer = (unsigned char *) (c_string_literal),       \
	})

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone primitive type operations.                                     │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
lone_u8  lone_u8_read (void *address);
lone_s8  lone_s8_read (void *address);
lone_u16 lone_u16_read(void *address);
lone_s16 lone_s16_read(void *address);
lone_u32 lone_u32_read(void *address);
lone_s32 lone_s32_read(void *address);
lone_u64 lone_u64_read(void *address);
lone_s64 lone_s64_read(void *address);

void lone_u8_write (void *address, lone_u8  u8);
void lone_s8_write (void *address, lone_s8  s8);
void lone_u16_write(void *address, lone_u16 u16);
void lone_s16_write(void *address, lone_s16 s16);
void lone_u32_write(void *address, lone_u32 u32);
void lone_s32_write(void *address, lone_s32 s32);
void lone_u64_write(void *address, lone_u64 u64);
void lone_s64_write(void *address, lone_s64 s64);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone bytes structure operations.                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
bool lone_bytes_equals(struct lone_bytes x, struct lone_bytes y);
bool lone_bytes_equals_c_string(struct lone_bytes bytes, char *c_string);
bool lone_bytes_contains_offset(struct lone_bytes bytes, lone_size offset);
bool lone_bytes_contains_block(struct lone_bytes bytes, lone_size offset, lone_size size);
bool lone_bytes_contains_address(struct lone_bytes bytes, void *pointer);

lone_u8  lone_bytes_read_u8 (struct lone_bytes bytes, lone_size offset);
lone_s8  lone_bytes_read_s8 (struct lone_bytes bytes, lone_size offset);
lone_u16 lone_bytes_read_u16(struct lone_bytes bytes, lone_size offset);
lone_s16 lone_bytes_read_s16(struct lone_bytes bytes, lone_size offset);
lone_u32 lone_bytes_read_u32(struct lone_bytes bytes, lone_size offset);
lone_s32 lone_bytes_read_s32(struct lone_bytes bytes, lone_size offset);
lone_u64 lone_bytes_read_u64(struct lone_bytes bytes, lone_size offset);
lone_s64 lone_bytes_read_s64(struct lone_bytes bytes, lone_size offset);

void lone_bytes_write_u8 (struct lone_bytes bytes, lone_size offset, lone_u8  u8);
void lone_bytes_write_s8 (struct lone_bytes bytes, lone_size offset, lone_s8  s8);
void lone_bytes_write_u16(struct lone_bytes bytes, lone_size offset, lone_u16 u16);
void lone_bytes_write_s16(struct lone_bytes bytes, lone_size offset, lone_s16 s16);
void lone_bytes_write_u32(struct lone_bytes bytes, lone_size offset, lone_u32 u32);
void lone_bytes_write_s32(struct lone_bytes bytes, lone_size offset, lone_s32 s32);
void lone_bytes_write_u64(struct lone_bytes bytes, lone_size offset, lone_u64 u64);
void lone_bytes_write_s64(struct lone_bytes bytes, lone_size offset, lone_s64 s64);

bool lone_bytes_checked_read_u8 (struct lone_bytes bytes, lone_size offset, lone_u8  *u8);
bool lone_bytes_checked_read_s8 (struct lone_bytes bytes, lone_size offset, lone_s8  *s8);
bool lone_bytes_checked_read_u16(struct lone_bytes bytes, lone_size offset, lone_u16 *u16);
bool lone_bytes_checked_read_s16(struct lone_bytes bytes, lone_size offset, lone_s16 *s16);
bool lone_bytes_checked_read_u32(struct lone_bytes bytes, lone_size offset, lone_u32 *u32);
bool lone_bytes_checked_read_s32(struct lone_bytes bytes, lone_size offset, lone_s32 *s32);
bool lone_bytes_checked_read_u64(struct lone_bytes bytes, lone_size offset, lone_u64 *u64);
bool lone_bytes_checked_read_s64(struct lone_bytes bytes, lone_size offset, lone_s64 *s64);

bool lone_bytes_checked_write_u8 (struct lone_bytes bytes, lone_size offset, lone_u8  value);
bool lone_bytes_checked_write_s8 (struct lone_bytes bytes, lone_size offset, lone_s8  value);
bool lone_bytes_checked_write_u16(struct lone_bytes bytes, lone_size offset, lone_u16 value);
bool lone_bytes_checked_write_s16(struct lone_bytes bytes, lone_size offset, lone_s16 value);
bool lone_bytes_checked_write_u32(struct lone_bytes bytes, lone_size offset, lone_u32 value);
bool lone_bytes_checked_write_s32(struct lone_bytes bytes, lone_size offset, lone_s32 value);
bool lone_bytes_checked_write_u64(struct lone_bytes bytes, lone_size offset, lone_u64 value);
bool lone_bytes_checked_write_s64(struct lone_bytes bytes, lone_size offset, lone_s64 value);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The lone system structure represents low level system state         │
   │    such as allocated memory and hash function state.                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_system {
	struct lone_memory *memory;
	struct {
		struct {
			unsigned long offset_basis;
		} fnv_1a;
	} hash;
};

/* ╭────────────────────┨ LONE LISP MEMORY ALLOCATION ┠─────────────────────╮
   │                                                                        │
   │    Lone implements a block-based memory allocator.                     │
   │    Memory blocks are allocated on a first fit basis.                   │
   │    They will be split into smaller units when allocated                │
   │    and merged together with free neighbors when deallocated.           │
   │                                                                        │
   │    Memory blocks are segments prefixed by a block descriptor           │
   │    that tracks its size, allocation status as well as pointers         │
   │    to surrounding memory blocks. It is simple to obtain a pointer      │
   │    to the block descriptor from a pointer to the memory block:         │
   │    simply subtract the block descriptor's size from the pointer.       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_memory {
	struct lone_memory *prev, *next;
	int free;
	size_t size;
	unsigned char pointer[];
};

#endif /* LONE_TYPES_HEADER */
