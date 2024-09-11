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

struct lone_optional_u8  { bool present; lone_u8  value; };
struct lone_optional_s8  { bool present; lone_s8  value; };
struct lone_optional_u16 { bool present; lone_u16 value; };
struct lone_optional_s16 { bool present; lone_s16 value; };
struct lone_optional_u32 { bool present; lone_u32 value; };
struct lone_optional_s32 { bool present; lone_s32 value; };
struct lone_optional_u64 { bool present; lone_u64 value; };
struct lone_optional_s64 { bool present; lone_s64 value; };

#define LONE_OPTIONAL_ABSENT_VALUE(__type) \
	((struct lone_optional_##__type) { .present = false, .value = 0 })
#define LONE_OPTIONAL_PRESENT_VALUE(__type, __value) \
	((struct lone_optional_##__type) { .present = true, .value = (__value) })

enum lone_c_type {
	LONE_TYPES_C_UNDEFINED = 0,

	LONE_TYPES_C_PLAIN_CHAR,
	LONE_TYPES_C_SIGNED_CHAR,
	LONE_TYPES_C_UNSIGNED_CHAR,

	LONE_TYPES_C_SIGNED_SHORT_INT,
	LONE_TYPES_C_UNSIGNED_SHORT_INT,

	LONE_TYPES_C_SIGNED_INT,
	LONE_TYPES_C_UNSIGNED_INT,

	LONE_TYPES_C_SIGNED_LONG_INT,
	LONE_TYPES_C_UNSIGNED_LONG_INT,

	LONE_TYPES_C_SIGNED_LONG_LONG_INT,
	LONE_TYPES_C_UNSIGNED_LONG_LONG_INT,

	LONE_TYPES_C_FLOAT,
	LONE_TYPES_C_DOUBLE,
	LONE_TYPES_C_LONG_DOUBLE,

	LONE_TYPES_C_BOOLEAN,

	LONE_TYPES_C_DATA_POINTER,
	LONE_TYPES_C_FUNCTION_POINTER,

	LONE_TYPES_C_S8,
	LONE_TYPES_C_S16,
	LONE_TYPES_C_S32,
	LONE_TYPES_C_S64,

	LONE_TYPES_C_U8,
	LONE_TYPES_C_U16,
	LONE_TYPES_C_U32,
	LONE_TYPES_C_U64,

	/* useful aliases */
	LONE_TYPES_C_POINTER               =    LONE_TYPES_C_DATA_POINTER,
	LONE_TYPES_C_BOOL                  =    LONE_TYPES_C_BOOLEAN,
	LONE_TYPES_C_CHAR                  =    LONE_TYPES_C_PLAIN_CHAR,
	LONE_TYPES_C_SHORT                 =    LONE_TYPES_C_SIGNED_SHORT_INT,
	LONE_TYPES_C_SIGNED_SHORT          =    LONE_TYPES_C_SIGNED_SHORT_INT,
	LONE_TYPES_C_UNSIGNED_SHORT        =    LONE_TYPES_C_UNSIGNED_SHORT_INT,
	LONE_TYPES_C_INT                   =    LONE_TYPES_C_SIGNED_INT,
	LONE_TYPES_C_LONG                  =    LONE_TYPES_C_SIGNED_LONG_INT,
	LONE_TYPES_C_SIGNED_LONG           =    LONE_TYPES_C_SIGNED_LONG_INT,
	LONE_TYPES_C_UNSIGNED_LONG         =    LONE_TYPES_C_UNSIGNED_LONG_INT,
	LONE_TYPES_C_LONG_LONG             =    LONE_TYPES_C_SIGNED_LONG_LONG_INT,
	LONE_TYPES_C_SIGNED_LONG_LONG      =    LONE_TYPES_C_SIGNED_LONG_LONG_INT,
	LONE_TYPES_C_UNSIGNED_LONG_LONG    =    LONE_TYPES_C_UNSIGNED_LONG_LONG_INT,
};

struct lone_c_values {
	union {
		char plain_char;
		signed char signed_char;
		unsigned char unsigned_char;

		signed short signed_short;
		unsigned short unsigned_short;

		signed int signed_int;
		unsigned int unsigned_int;

		signed long signed_long;
		unsigned long unsigned_long;

		signed long long signed_long_long;
		unsigned long long unsigned_long_long;

		float single_precision;
		double double_precision;
		long double quadruple_precision;

		bool boolean;

		void *pointer;
		void (*function_pointer)(void);

		lone_s8   s8;
		lone_s16 s16;
		lone_s32 s32;
		lone_s64 s64;

		lone_u8   u8;
		lone_u16 u16;
		lone_u32 u32;
		lone_u64 u64;
	} as;
};

struct lone_c_value {
	enum   lone_c_type   type;
	struct lone_c_values value;
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

#define LONE_BYTES_INIT(__count, __pointer) { .count = (__count), .pointer = (__pointer) }
#define LONE_BYTES_INIT_NULL() { .count = 0, .pointer = 0 }
#define LONE_BYTES_INIT_FROM_LITERAL(__c_string_literal) \
	{ \
		.count = sizeof(__c_string_literal) - 1, \
		.pointer = ((unsigned char *) (__c_string_literal)) \
	}

#define LONE_BYTES_VALUE(__count, __pointer)                                                       \
	((struct lone_bytes) {                                                                     \
		.count = (__count),                                                                \
		.pointer = ((unsigned char *) (__pointer)),                                        \
	})

#define LONE_BYTES_VALUE_NULL()                                                                    \
	((struct lone_bytes) {                                                                     \
		.count = 0,                                                                        \
		.pointer = 0,                                                                      \
	})

#define LONE_BYTES_VALUE_FROM_LITERAL(__c_string_literal)                                          \
	((struct lone_bytes) {                                                                     \
		.count = sizeof(__c_string_literal) - 1,                                           \
		.pointer = ((unsigned char *) (__c_string_literal)),                               \
	})

#define LONE_BYTES_VALUE_FROM_ARRAY(__array)                                                       \
	((struct lone_bytes) {                                                                     \
		.count = sizeof(__array),                                                          \
		.pointer = ((unsigned char *) (__array)),                                          \
	})

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone primitive type operations.                                     │
   │                                                                        │
   │    Supports reading/writing all lone integer types from/to memory      │
   │    at arbitrary addresses, aligned or not. If not specified,           │
   │    native endianness is implied. Big/little endian variants            │
   │    are also provided, also with support for unaligned addresses.       │
   │                                                                        │
   │    Useful for avoiding undefined behavior when accessing memory.       │
   │    Normal C pointers provide efficient access to aligned addresses.    │
   │    Linux also exports efficient byte swapping functions in the UAPI    │
   │    headers which also require aligned pointers.                        │
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

lone_u16 lone_u16le_read(void *address);
lone_s16 lone_s16le_read(void *address);
lone_u32 lone_u32le_read(void *address);
lone_s32 lone_s32le_read(void *address);
lone_u64 lone_u64le_read(void *address);
lone_s64 lone_s64le_read(void *address);

lone_u16 lone_u16be_read(void *address);
lone_s16 lone_s16be_read(void *address);
lone_u32 lone_u32be_read(void *address);
lone_s32 lone_s32be_read(void *address);
lone_u64 lone_u64be_read(void *address);
lone_s64 lone_s64be_read(void *address);

void lone_u16le_write(void *address, lone_u16 u16);
void lone_s16le_write(void *address, lone_s16 s16);
void lone_u32le_write(void *address, lone_u32 u32);
void lone_s32le_write(void *address, lone_s32 s32);
void lone_u64le_write(void *address, lone_u64 u64);
void lone_s64le_write(void *address, lone_s64 s64);

void lone_u16be_write(void *address, lone_u16 u16);
void lone_s16be_write(void *address, lone_s16 s16);
void lone_u32be_write(void *address, lone_u32 u32);
void lone_s32be_write(void *address, lone_s32 s32);
void lone_u64be_write(void *address, lone_u64 u64);
void lone_s64be_write(void *address, lone_s64 s64);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone bytes structure operations.                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
bool lone_bytes_is_equal(struct lone_bytes x, struct lone_bytes y);
bool lone_bytes_is_equal_to_c_string(struct lone_bytes bytes, char *c_string);
bool lone_bytes_is_zero(struct lone_bytes b);
bool lone_bytes_contains_offset(struct lone_bytes bytes, lone_size offset);
bool lone_bytes_contains_block(struct lone_bytes bytes, lone_size offset, lone_size size);
bool lone_bytes_contains_address(struct lone_bytes bytes, void *pointer);

struct lone_optional_u8  lone_bytes_read_u8 (struct lone_bytes bytes, lone_size offset);
struct lone_optional_s8  lone_bytes_read_s8 (struct lone_bytes bytes, lone_size offset);
struct lone_optional_u16 lone_bytes_read_u16(struct lone_bytes bytes, lone_size offset);
struct lone_optional_s16 lone_bytes_read_s16(struct lone_bytes bytes, lone_size offset);
struct lone_optional_u32 lone_bytes_read_u32(struct lone_bytes bytes, lone_size offset);
struct lone_optional_s32 lone_bytes_read_s32(struct lone_bytes bytes, lone_size offset);
struct lone_optional_u64 lone_bytes_read_u64(struct lone_bytes bytes, lone_size offset);
struct lone_optional_s64 lone_bytes_read_s64(struct lone_bytes bytes, lone_size offset);

bool lone_bytes_write_u8 (struct lone_bytes bytes, lone_size offset, lone_u8  u8);
bool lone_bytes_write_s8 (struct lone_bytes bytes, lone_size offset, lone_s8  s8);
bool lone_bytes_write_u16(struct lone_bytes bytes, lone_size offset, lone_u16 u16);
bool lone_bytes_write_s16(struct lone_bytes bytes, lone_size offset, lone_s16 s16);
bool lone_bytes_write_u32(struct lone_bytes bytes, lone_size offset, lone_u32 u32);
bool lone_bytes_write_s32(struct lone_bytes bytes, lone_size offset, lone_s32 s32);
bool lone_bytes_write_u64(struct lone_bytes bytes, lone_size offset, lone_u64 u64);
bool lone_bytes_write_s64(struct lone_bytes bytes, lone_size offset, lone_s64 s64);

struct lone_optional_u16 lone_bytes_read_u16le(struct lone_bytes bytes, lone_size offset);
struct lone_optional_s16 lone_bytes_read_s16le(struct lone_bytes bytes, lone_size offset);
struct lone_optional_u32 lone_bytes_read_u32le(struct lone_bytes bytes, lone_size offset);
struct lone_optional_s32 lone_bytes_read_s32le(struct lone_bytes bytes, lone_size offset);
struct lone_optional_u64 lone_bytes_read_u64le(struct lone_bytes bytes, lone_size offset);
struct lone_optional_s64 lone_bytes_read_s64le(struct lone_bytes bytes, lone_size offset);

bool lone_bytes_write_u16le(struct lone_bytes bytes, lone_size offset, lone_u16 u16);
bool lone_bytes_write_s16le(struct lone_bytes bytes, lone_size offset, lone_s16 s16);
bool lone_bytes_write_u32le(struct lone_bytes bytes, lone_size offset, lone_u32 u32);
bool lone_bytes_write_s32le(struct lone_bytes bytes, lone_size offset, lone_s32 s32);
bool lone_bytes_write_u64le(struct lone_bytes bytes, lone_size offset, lone_u64 u64);
bool lone_bytes_write_s64le(struct lone_bytes bytes, lone_size offset, lone_s64 s64);

struct lone_optional_u16 lone_bytes_read_u16be(struct lone_bytes bytes, lone_size offset);
struct lone_optional_s16 lone_bytes_read_s16be(struct lone_bytes bytes, lone_size offset);
struct lone_optional_u32 lone_bytes_read_u32be(struct lone_bytes bytes, lone_size offset);
struct lone_optional_s32 lone_bytes_read_s32be(struct lone_bytes bytes, lone_size offset);
struct lone_optional_u64 lone_bytes_read_u64be(struct lone_bytes bytes, lone_size offset);
struct lone_optional_s64 lone_bytes_read_s64be(struct lone_bytes bytes, lone_size offset);

bool lone_bytes_write_u16be(struct lone_bytes bytes, lone_size offset, lone_u16 u16);
bool lone_bytes_write_s16be(struct lone_bytes bytes, lone_size offset, lone_s16 s16);
bool lone_bytes_write_u32be(struct lone_bytes bytes, lone_size offset, lone_u32 u32);
bool lone_bytes_write_s32be(struct lone_bytes bytes, lone_size offset, lone_s32 s32);
bool lone_bytes_write_u64be(struct lone_bytes bytes, lone_size offset, lone_u64 u64);
bool lone_bytes_write_s64be(struct lone_bytes bytes, lone_size offset, lone_s64 s64);

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
