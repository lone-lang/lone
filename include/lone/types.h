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

typedef long lone_signed_integer;
typedef unsigned long lone_unsigned_integer;
typedef lone_signed_integer lone_integer;

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

struct lone_reader {
	int file_descriptor;
	struct {
		struct lone_bytes bytes;
		struct {
			size_t read;
			size_t write;
		} position;
	} buffer;
	struct {
		bool end_of_input: 1;
		bool error: 1;
	} status;
};

/* ╭──────────────────────────┨ LONE LISP TYPES ┠───────────────────────────╮
   │                                                                        │
   │    Lone implements dynamic data types as a tagged union.               │
   │    Supported types are:                                                │
   │                                                                        │
   │        ◦ Module       the isolated programming environment type        │
   │        ◦ Function     the reusable executable expressions type         │
   │        ◦ Primitive    the built-in C subroutine type                   │
   │        ◦ List         the linked list and tree type                    │
   │        ◦ Vector       the contiguous array of values type              │
   │        ◦ Table        the hash table, prototype and object type        │
   │        ◦ Symbol       the keyword and interned string type             │
   │        ◦ Text         the UTF-8 encoded text type                      │
   │        ◦ Bytes        the binary data and low level string type        │
   │        ◦ Pointer      the memory addressing and dereferencing type     │
   │        ◦ Integer      the signed integer type                          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

enum lone_value_type {
	LONE_NIL = 0,
	LONE_HEAP_VALUE,
	LONE_INTEGER,
	LONE_POINTER,
};

enum lone_heap_value_type {
	LONE_MODULE,
	LONE_FUNCTION,
	LONE_PRIMITIVE,
	LONE_LIST,
	LONE_VECTOR,
	LONE_TABLE,
	LONE_SYMBOL,
	LONE_TEXT,
	LONE_BYTES,
};

enum lone_pointer_type {
	LONE_TO_UNKNOWN,

	LONE_TO_U8,  LONE_TO_S8,
	LONE_TO_U16, LONE_TO_S16,
	LONE_TO_U32, LONE_TO_S32,
	LONE_TO_U64, LONE_TO_S64,
};

union lone_pointer {
	void *to_void;
	char *to_char;

	float  *to_float;
	double *to_double;

	signed char        *to_signed_char;
	signed short       *to_signed_short;
	signed int         *to_signed_int;
	signed long        *to_signed_long;
	signed long long   *to_signed_long_long;

	unsigned char      *to_unsigned_char;
	unsigned short     *to_unsigned_short;
	unsigned int       *to_unsigned_int;
	unsigned long      *to_unsigned_long;
	unsigned long long *to_unsigned_long_long;

	lone_s8  *to_s8;
	lone_s16 *to_s16;
	lone_s32 *to_s32;
	lone_s64 *to_s64;

	lone_u8  *to_u8;
	lone_u16 *to_u16;
	lone_u32 *to_u32;
	lone_u64 *to_u64;

	lone_signed_integer *to_signed_integer;
	lone_unsigned_integer *to_unsigned_integer;
	lone_integer *to_integer;
};

struct lone_heap_value;
struct lone_value {
	union {
		struct lone_heap_value *heap_value;
		union lone_pointer pointer;
		lone_signed_integer signed_integer;
		lone_unsigned_integer unsigned_integer;
		lone_integer integer;
	} as;

	struct {
		enum lone_value_type type;
		enum lone_pointer_type pointer_type;
	};
};

struct lone_module {
	struct lone_value name;
	struct lone_value environment;
	struct lone_value exports;
};

/* https://dl.acm.org/doi/10.1145/947941.947948
 * https://user.ceng.metu.edu.tr/~ucoluk/research/lisp/lispman/node24.html
 */
struct lone_function_flags {
	bool evaluate_arguments: 1;
	bool evaluate_result: 1;
};

struct lone_function {
	struct lone_value arguments;         /* the bindings */
	struct lone_value code;              /* the lambda */
	struct lone_value environment;       /* the closure */
	struct lone_function_flags flags;    /* how to evaluate & apply */
};

struct lone_lisp;
typedef struct lone_value (*lone_primitive)(struct lone_lisp *lone,
                                            struct lone_value module,
                                            struct lone_value environment,
                                            struct lone_value arguments,
                                            struct lone_value closure);

struct lone_primitive {
	struct lone_value name;
	lone_primitive function;
	struct lone_value closure;
	struct lone_function_flags flags;
};

struct lone_list {
	struct lone_value first;
	struct lone_value rest;
};

struct lone_vector {
	struct lone_value *values;
	size_t count;
	size_t capacity;
};

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone tables are openly addressed, linearly probed hash tables.      │
   │                                                                        │
   │    Entries are stored compactly and maintain insertion order.          │
   │    Hashes index into a separate sparse array of indexes                │
   │    meant for the compact entries array.                                │
   │                                                                        │
   │    Tables strive to maintain a load factor of at most 0.7:             │
   │    they will be rehashed once they're above 70% capacity.              │
   │                                                                        │
   │    When deleting keys, entries are shifted backwards.                  │
   │    Tombstones are not used.                                            │
   │                                                                        │
   │    Currently, lone tables use the FNV-1a hashing algorithm.            │
   │    More algorithms will probably be implemented in the future.         │
   │                                                                        │
   │    Tables are able to inherit from another table:                      │
   │    missing keys are also looked up in the parent table.                │
   │    This is currently used to implement nested environments             │
   │    but will also serve as a prototype-based object system              │
   │    as in Javascript and Self.                                          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_table_index {
	bool used: 1;
	size_t index;
};

struct lone_table_entry {
	struct lone_value key;
	struct lone_value value;
};

struct lone_table {
	size_t count;
	size_t capacity;
	struct lone_table_index *indexes;
	struct lone_table_entry *entries;
	struct lone_value prototype;
};

struct lone_heap_value {
	struct {
		bool live: 1;
		bool marked: 1;
		bool should_deallocate_bytes: 1;
	};

	enum lone_heap_value_type type;

	union {
		struct lone_module module;
		struct lone_function function;
		struct lone_primitive primitive;
		struct lone_list list;
		struct lone_vector vector;
		struct lone_table table;
		struct lone_bytes bytes;   /* also used by texts and symbols */
	} as;
};

typedef bool (*lone_predicate)(struct lone_value value);
typedef bool (*lone_comparator)(struct lone_value x, struct lone_value y);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Singleton values.                                                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value lone_nil(void);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Type predicate functions.                                           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

bool lone_is_register_value(struct lone_value value);
bool lone_is_heap_value(struct lone_value value);
bool lone_is_register_value_of_type(struct lone_value value, enum lone_value_type register_value_type);
bool lone_is_heap_value_of_type(struct lone_value value, enum lone_heap_value_type heap_value_type);

bool lone_is_module(struct lone_value value);
bool lone_is_function(struct lone_value value);
bool lone_is_primitive(struct lone_value value);
bool lone_is_applicable(struct lone_value value);
bool lone_is_list(struct lone_value value);
bool lone_is_list_or_nil(struct lone_value value);
bool lone_is_vector(struct lone_value value);
bool lone_is_table(struct lone_value value);
bool lone_has_bytes(struct lone_value value);
bool lone_is_bytes(struct lone_value value);
bool lone_is_text(struct lone_value value);
bool lone_is_symbol(struct lone_value value);

bool lone_is_nil(struct lone_value value);
bool lone_is_integer(struct lone_value value);
bool lone_is_pointer(struct lone_value value);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Comparison and equality functions.                                  │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
bool lone_has_same_type(struct lone_value x, struct lone_value y);
bool lone_is_identical(struct lone_value x, struct lone_value y);
bool lone_is_equivalent(struct lone_value x, struct lone_value y);
bool lone_is_equal(struct lone_value x, struct lone_value y);
bool lone_integer_is_less_than(struct lone_value x, struct lone_value y);
bool lone_integer_is_less_than_or_equal_to(struct lone_value x, struct lone_value y);
bool lone_integer_is_greater_than(struct lone_value x, struct lone_value y);
bool lone_integer_is_greater_than_or_equal_to(struct lone_value x, struct lone_value y);

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

bool lone_bytes_equals(struct lone_bytes x, struct lone_bytes y);
bool lone_bytes_equals_c_string(struct lone_bytes bytes, char *c_string);

/* ╭───────────────────────┨ LONE LISP INTERPRETER ┠────────────────────────╮
   │                                                                        │
   │    The lone lisp interpreter is composed of all internal state         │
   │    necessary to process useful programs. It includes memory,           │
   │    references to all allocated objects, a table of interned            │
   │    symbols, references to constant values such as nil and              │
   │    a table of loaded modules and the top level null module.            │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp {
	struct {
		void *stack;
		struct lone_memory *general;
		struct lone_heap *heaps;
	} memory;
	struct lone_value symbol_table;
	struct {
		struct lone_value truth;
	} constants;
	struct {
		struct lone_value loaded;
		struct lone_value embedded;
		struct lone_value null;
		struct lone_value top_level_environment;
		struct lone_value path;
	} modules;
	struct {
		struct {
			unsigned long offset_basis;
		} fnv_1a;
	} hash;
};

/* ╭────────────────────┨ LONE LISP MEMORY ALLOCATION ┠─────────────────────╮
   │                                                                        │
   │    Lone is designed to work without any dependencies except Linux,     │
   │    so it does not make use of even the system's C library.             │
   │    In order to bootstrap itself in such harsh conditions,              │
   │    it must be given some memory to work with.                          │
   │                                                                        │
   │    Lone manages its own memory with a block-based allocator.           │
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
   │    All allocated lone lisp values are placed in the value heap,        │
   │    essentially a linked list of arrays of contiguous lone values.      │
   │                                                                        │
   │    Lone employs a very simple mark-and-sweep garbage collector.        │
   │    It walks through the whole interpreter, including the processor     │
   │    stack and registers, looking for pointers that fall within the      │
   │    ranges of each allocated value heap and marking them as used.       │
   │    The purpose of the heap is to enable more efficient pointer         │
   │    ranging computations so as to avoid the need to quadratically       │
   │    check every discovered pointer against every managed pointer.       │
   │    Once all reachable pointers are marked, the garbage collector       │
   │    sweeps through the value heap, killing all unmarked values          │
   │    by marking them as unused. Future lone value allocations            │
   │    may simply return these objects, thereby resurrecting them.         │
   │                                                                        │
   │    When a value heap is allocated, all values within it are dead.      │
   │    After each garbage collection cycle, completely dead heaps          │
   │    are deallocated, thereby freeing up memory for other uses.          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_memory {
	struct lone_memory *prev, *next;
	int free;
	size_t size;
	unsigned char pointer[];
};

struct lone_heap {
	struct lone_heap *next;
	struct lone_heap_value values[LONE_MEMORY_HEAP_VALUE_COUNT];
};

#endif /* LONE_TYPES_HEADER */
