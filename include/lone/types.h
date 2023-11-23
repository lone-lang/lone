/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_TYPES_HEADER
#define LONE_TYPES_HEADER

#include <stdbool.h>
#include <stdint.h>

#include <linux/types.h>
#include <linux/elf.h>

typedef __kernel_size_t size_t;
typedef __kernel_ssize_t ssize_t;
typedef __kernel_off_t off_t;

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
		char *c_string;
		void *pointer;
		long integer;
		unsigned long unsigned_integer;
	} as;
};

struct lone_auxiliary_vector {
	long type;
	struct lone_auxiliary_value value;
};

struct lone_bytes {
	size_t count;
	unsigned char *pointer;
};

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
	struct lone_value *symbol_table;
	struct {
		struct lone_value *nil;
		struct lone_value *truth;
	} constants;
	struct {
		struct lone_value *loaded;
		struct lone_value *null;
		struct lone_value *top_level_environment;
		struct lone_value *path;
	} modules;
	struct {
		struct {
			unsigned long offset_basis;
		} fnv_1a;
	} hash;
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
	int error;
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

enum lone_type {
	LONE_MODULE,
	LONE_FUNCTION,
	LONE_PRIMITIVE,
	LONE_LIST,
	LONE_VECTOR,
	LONE_TABLE,
	LONE_SYMBOL,
	LONE_TEXT,
	LONE_BYTES,
	LONE_POINTER,
	LONE_INTEGER,
};

enum lone_pointer_type {
	LONE_TO_UNKNOWN,

	LONE_TO_U8,  LONE_TO_I8,
	LONE_TO_U16, LONE_TO_I16,
	LONE_TO_U32, LONE_TO_I32,
	LONE_TO_U64, LONE_TO_I64,
};

struct lone_module {
	struct lone_value *name;
	struct lone_value *environment;
	struct lone_value *exports;
};

/* https://dl.acm.org/doi/10.1145/947941.947948
 * https://user.ceng.metu.edu.tr/~ucoluk/research/lisp/lispman/node24.html
 */
struct lone_function_flags {
	bool evaluate_arguments: 1;
	bool evaluate_result: 1;
	bool variable_arguments: 1;
};

struct lone_function {
	struct lone_value *arguments;        /* the bindings */
	struct lone_value *code;             /* the lambda */
	struct lone_value *environment;      /* the closure */
	struct lone_function_flags flags;    /* how to evaluate & apply */
};

typedef struct lone_value *(*lone_primitive)(struct lone_lisp *lone,
                                             struct lone_value *module,
                                             struct lone_value *environment,
                                             struct lone_value *arguments,
                                             struct lone_value *closure);

struct lone_primitive {
	struct lone_value *name;
	lone_primitive function;
	struct lone_value *closure;
	struct lone_function_flags flags;    /* primitives always accept variable arguments */
};

struct lone_list {
	struct lone_value *first;
	struct lone_value *rest;
};

struct lone_vector {
	struct lone_value **values;
	size_t count;
	size_t capacity;
};

struct lone_table_entry {
	struct lone_value *key;
	struct lone_value *value;
};

struct lone_table {
	size_t count;
	size_t capacity;
	struct lone_table_entry *entries;
	struct lone_value *prototype;
};

struct lone_pointer {
	enum lone_pointer_type type;
	void *address;
};

struct lone_value {
	struct {
		bool live: 1;
		bool marked: 1;
		bool should_deallocate_bytes: 1;
	};

	enum lone_type type;

	union {
		struct lone_module module;
		struct lone_function function;
		struct lone_primitive primitive;
		struct lone_list list;
		struct lone_vector vector;
		struct lone_table table;
		struct lone_bytes bytes;   /* also used by texts and symbols */
		struct lone_pointer pointer;
		long integer;
	};
};

typedef bool (*lone_predicate)(struct lone_value *);
typedef bool (*lone_comparator)(struct lone_value *, struct lone_value *);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Type predicate functions.                                           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

bool lone_has_same_type(struct lone_value *x, struct lone_value *y);
bool lone_is_module(struct lone_value *value);
bool lone_is_function(struct lone_value *value);
bool lone_is_primitive(struct lone_value *value);
bool lone_is_applicable(struct lone_value *value);
bool lone_is_list(struct lone_value *value);
bool lone_is_vector(struct lone_value *value);
bool lone_is_table(struct lone_value *value);
bool lone_is_nil(struct lone_value *value);
bool lone_has_bytes(struct lone_value *value);
bool lone_is_bytes(struct lone_value *value);
bool lone_is_text(struct lone_value *value);
bool lone_is_symbol(struct lone_value *value);
bool lone_is_integer(struct lone_value *value);
bool lone_is_pointer(struct lone_value *value);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Comparison and equality functions.                                  │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
bool lone_is_identical(struct lone_value *x, struct lone_value *y);
bool lone_is_equivalent(struct lone_value *x, struct lone_value *y);
bool lone_is_equal(struct lone_value *x, struct lone_value *y);
bool lone_list_is_equal(struct lone_value *x, struct lone_value *y);
bool lone_vector_is_equal(struct lone_value *x, struct lone_value *y);
bool lone_table_is_equal(struct lone_value *x, struct lone_value *y);
bool lone_integer_is_less_than(struct lone_value *x, struct lone_value *y);
bool lone_integer_is_less_than_or_equal_to(struct lone_value *x, struct lone_value *y);
bool lone_integer_is_greater_than(struct lone_value *x, struct lone_value *y);
bool lone_integer_is_greater_than_or_equal_to(struct lone_value *x, struct lone_value *y);
bool lone_bytes_equals(struct lone_bytes x, struct lone_bytes y);
bool lone_bytes_equals_c_string(struct lone_bytes bytes, char *c_string);

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
   │    to surrounding memory blocks.                                       │
   │                                                                        │
   │    Lone employs a very simple mark-and-sweep garbage collector.        │
   │    It starts by marking all values reachable by the interpreter        │
   │    in its current state. Then it walks the list of all values,         │
   │    deallocating any unmarked object it finds as well as any            │
   │    memory they happen own based on the value's type.                   │
   │                                                                        │
   │    Like memory blocks, lone values are prefixed by a header            │
   │    structure containing metadata such as its marked state              │
   │    as well as a pointer to the next object in the list.                │
   │                                                                        │
   │    Since these headers are prefixed to pointers returned by            │
   │    allocation and value creation functions, it is simple to            │
   │    calculate their locations given a pointer to a memory block         │
   │    or lone value: simply subtract the header's size from it.           │
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
	size_t count;
	struct lone_value values[];
};

#endif /* LONE_TYPES_HEADER */
