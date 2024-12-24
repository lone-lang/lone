/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_TYPES_HEADER
#define LONE_LISP_TYPES_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

#include <lone/lisp/definitions.h>
#include <lone/lisp/types.h>

typedef long lone_lisp_integer;

struct lone_lisp_reader {
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

enum lone_lisp_value_type {
	LONE_LISP_TYPE_NIL = 0,
	LONE_LISP_TYPE_HEAP_VALUE,
	LONE_LISP_TYPE_INTEGER,
	LONE_LISP_TYPE_POINTER,
};

enum lone_lisp_heap_value_type {
	LONE_LISP_TYPE_MODULE,
	LONE_LISP_TYPE_FUNCTION,
	LONE_LISP_TYPE_PRIMITIVE,
	LONE_LISP_TYPE_LIST,
	LONE_LISP_TYPE_VECTOR,
	LONE_LISP_TYPE_TABLE,
	LONE_LISP_TYPE_SYMBOL,
	LONE_LISP_TYPE_TEXT,
	LONE_LISP_TYPE_BYTES,
};

struct lone_lisp_heap_value;
struct lone_lisp_value {
	union {
		struct lone_lisp_heap_value *heap_value;
		lone_lisp_integer integer;
		void *pointer;
	} as;

	enum lone_lisp_value_type type;
};

struct lone_lisp_module {
	struct lone_lisp_value name;
	struct lone_lisp_value environment;
	struct lone_lisp_value exports;
};

/* https://dl.acm.org/doi/10.1145/947941.947948
 * https://user.ceng.metu.edu.tr/~ucoluk/research/lisp/lispman/node24.html
 */
struct lone_lisp_function_flags {
	bool evaluate_arguments: 1;
	bool evaluate_result: 1;
};

struct lone_lisp_function {
	struct lone_lisp_value arguments;         /* the bindings */
	struct lone_lisp_value code;              /* the lambda */
	struct lone_lisp_value environment;       /* the closure */
	struct lone_lisp_function_flags flags;    /* how to evaluate & apply */
};

struct lone_lisp;

typedef struct lone_lisp_value (*lone_lisp_primitive_function)(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value environment,
		struct lone_lisp_value arguments, struct lone_lisp_value closure);

struct lone_lisp_primitive {
	struct lone_lisp_value name;
	lone_lisp_primitive_function function;
	struct lone_lisp_value closure;
	struct lone_lisp_function_flags flags;
};

struct lone_lisp_list {
	struct lone_lisp_value first;
	struct lone_lisp_value rest;
};

struct lone_lisp_vector {
	struct lone_lisp_value *values;
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
struct lone_lisp_table_index {
	bool used: 1;
	size_t index;
};

struct lone_lisp_table_entry {
	struct lone_lisp_value key;
	struct lone_lisp_value value;
};

struct lone_lisp_table {
	size_t count;
	size_t capacity;
	struct lone_lisp_table_index *indexes;
	struct lone_lisp_table_entry *entries;
	struct lone_lisp_value prototype;
};

struct lone_lisp_heap_value {
	struct {
		bool live: 1;
		bool marked: 1;
		bool should_deallocate_bytes: 1;
	};

	enum lone_lisp_heap_value_type type;

	union {
		struct lone_lisp_module module;
		struct lone_lisp_function function;
		struct lone_lisp_primitive primitive;
		struct lone_lisp_list list;
		struct lone_lisp_vector vector;
		struct lone_lisp_table table;
		struct lone_bytes bytes;   /* also used by texts and symbols */
	} as;
};

typedef bool (*lone_lisp_predicate_function)(struct lone_lisp_value value);
typedef bool (*lone_lisp_comparator_function)(struct lone_lisp_value x, struct lone_lisp_value y);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Singleton values.                                                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_nil(void);
struct lone_lisp_value lone_lisp_boolean_for(struct lone_lisp *lisp, bool value);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Type predicate functions.                                           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

enum lone_lisp_value_type lone_lisp_value_to_type(struct lone_lisp_value value);
struct lone_lisp_heap_value *lone_lisp_value_to_heap_value(struct lone_lisp_value value);
lone_lisp_integer lone_lisp_value_to_integer(struct lone_lisp_value value);
void *lone_lisp_value_to_pointer(struct lone_lisp_value value);

bool lone_lisp_is_register_value(struct lone_lisp_value value);
bool lone_lisp_is_heap_value(struct lone_lisp_value value);
bool lone_lisp_is_register_value_of_type(struct lone_lisp_value value, enum lone_lisp_value_type register_value_type);
bool lone_lisp_is_heap_value_of_type(struct lone_lisp_value value, enum lone_lisp_heap_value_type heap_value_type);

bool lone_lisp_is_module(struct lone_lisp_value value);
bool lone_lisp_is_function(struct lone_lisp_value value);
bool lone_lisp_is_primitive(struct lone_lisp_value value);
bool lone_lisp_is_applicable(struct lone_lisp_value value);
bool lone_lisp_is_list(struct lone_lisp_value value);
bool lone_lisp_is_list_or_nil(struct lone_lisp_value value);
bool lone_lisp_is_vector(struct lone_lisp_value value);
bool lone_lisp_is_table(struct lone_lisp_value value);
bool lone_lisp_has_bytes(struct lone_lisp_value value);
bool lone_lisp_is_bytes(struct lone_lisp_value value);
bool lone_lisp_is_text(struct lone_lisp_value value);
bool lone_lisp_is_symbol(struct lone_lisp_value value);

bool lone_lisp_is_nil(struct lone_lisp_value value);
bool lone_lisp_is_integer(struct lone_lisp_value value);
bool lone_lisp_is_pointer(struct lone_lisp_value value);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Comparison and equality functions.                                  │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
bool lone_lisp_has_same_type(struct lone_lisp_value x, struct lone_lisp_value y);
bool lone_lisp_is_identical(struct lone_lisp_value x, struct lone_lisp_value y);
bool lone_lisp_is_equivalent(struct lone_lisp_value x, struct lone_lisp_value y);
bool lone_lisp_is_equal(struct lone_lisp_value x, struct lone_lisp_value y);
bool lone_lisp_integer_is_less_than(struct lone_lisp_value x, struct lone_lisp_value y);
bool lone_lisp_integer_is_less_than_or_equal_to(struct lone_lisp_value x, struct lone_lisp_value y);
bool lone_lisp_integer_is_greater_than(struct lone_lisp_value x, struct lone_lisp_value y);
bool lone_lisp_integer_is_greater_than_or_equal_to(struct lone_lisp_value x, struct lone_lisp_value y);

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
	struct lone_system *system;
	void *native_stack;
	struct lone_lisp_heap *heaps;
	struct lone_lisp_value symbol_table;
	struct {
		struct lone_lisp_value truth;
	} constants;
	struct {
		struct lone_lisp_value loaded;
		struct lone_lisp_value embedded;
		struct lone_lisp_value null;
		struct lone_lisp_value top_level_environment;
		struct lone_lisp_value path;
	} modules;
};

/* ╭────────────────────┨ LONE LISP MEMORY ALLOCATION ┠─────────────────────╮
   │                                                                        │
   │    Lone is designed to work without any dependencies except Linux,     │
   │    so it does not make use of even the system's C library.             │
   │    In order to bootstrap itself in such harsh conditions,              │
   │    the underlying lone system must be initialized with a pointer       │
   │    to a readable and writable memory area of reasonable size,          │
   │    usually a statically allocated byte array.                          │
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

struct lone_lisp_heap {
	struct lone_lisp_heap *next;
	struct lone_lisp_heap_value values[LONE_LISP_HEAP_VALUE_COUNT];
};

#endif /* LONE_LISP_TYPES_HEADER */
