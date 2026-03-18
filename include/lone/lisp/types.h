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
   │        ◦ Module          isolated programming environment type         │
   │        ◦ Function        reusable executable expressions type          │
   │        ◦ Primitive       built-in C subroutine type                    │
   │        ◦ Continuation    callable captured stack frame type            │
   │        ◦ List            linked list and tree type                     │
   │        ◦ Vector          contiguous array of values type               │
   │        ◦ Table           hash table, prototype and object type         │
   │        ◦ Symbol          keyword and interned string type              │
   │        ◦ Text            UTF-8 encoded text type                       │
   │        ◦ Bytes           binary data and low level string type         │
   │        ◦ Integer         signed integer type                           │
   │        ◦ True            true constant                                 │
   │        ◦ False           false constant                                │
   │        ◦ Nil             nil constant                                  │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

enum lone_lisp_heap_value_type {
	LONE_LISP_TYPE_MODULE,
	LONE_LISP_TYPE_FUNCTION,
	LONE_LISP_TYPE_PRIMITIVE,
	LONE_LISP_TYPE_CONTINUATION,
	LONE_LISP_TYPE_GENERATOR,
	LONE_LISP_TYPE_LIST,
	LONE_LISP_TYPE_VECTOR,
	LONE_LISP_TYPE_TABLE,
	LONE_LISP_TYPE_SYMBOL,
	LONE_LISP_TYPE_TEXT,
	LONE_LISP_TYPE_BYTES,
};

enum lone_lisp_value_type {
	LONE_LISP_TYPE_HEAP_VALUE =                       (0 << 2) | (0 << 1) | (0 << 0), /*    000 */
	LONE_LISP_TYPE_INTEGER    =                       (0 << 2) | (0 << 1) | (1 << 0), /*    001 */
	LONE_LISP_TYPE_NIL        = (0 << 4) | (0 << 3) | (1 << 2) | (1 << 1) | (1 << 0), /* 00 111 */
	LONE_LISP_TYPE_TRUE       = (0 << 4) | (1 << 3) | (1 << 2) | (1 << 1) | (1 << 0), /* 01 111 */
	LONE_LISP_TYPE_FALSE      = (1 << 4) | (0 << 3) | (1 << 2) | (1 << 1) | (1 << 0), /* 10 111 */
};

struct lone_lisp_value {
	long tagged;
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
struct lone_lisp_machine;

/* arguments are pushed as a list on the lone lisp machine stack
 * return values are pushed on the lone lisp machine stack */
typedef long (*lone_lisp_primitive_function)(struct lone_lisp *lone, struct lone_lisp_machine *machine, long step);

struct lone_lisp_primitive {
	struct lone_lisp_value name;
	lone_lisp_primitive_function function;
	struct lone_lisp_value closure;
	struct lone_lisp_function_flags flags;
};

struct lone_lisp_machine_stack_frame;
enum lone_lisp_machine_step;

struct lone_lisp_continuation {
	size_t frame_count;
	struct lone_lisp_machine_stack_frame *frames;
};

struct lone_lisp_machine_stack {
	struct lone_lisp_machine_stack_frame *base;
	struct lone_lisp_machine_stack_frame *top;
	struct lone_lisp_machine_stack_frame *limit;
};

struct lone_lisp_generator {
	struct lone_lisp_value function;
	struct {
		struct lone_lisp_machine_stack own;
		struct lone_lisp_machine_stack caller;
	} stacks;
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
		struct lone_lisp_continuation continuation;
		struct lone_lisp_generator generator;
		struct lone_lisp_list list;
		struct lone_lisp_vector vector;
		struct lone_lisp_table table;
		struct lone_bytes bytes;   /* also used by texts and symbols */
	} as;
} __attribute__((aligned(64)));

typedef bool (*lone_lisp_predicate_function)(struct lone_lisp *lone, struct lone_lisp_value value);
typedef bool (*lone_lisp_comparator_function)(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Singleton values.                                                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_nil(void);
struct lone_lisp_value lone_lisp_false(void);
struct lone_lisp_value lone_lisp_true(void);
struct lone_lisp_value lone_lisp_boolean_for(bool value);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Type predicate functions.                                           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

unsigned char lone_lisp_type_tag_of(struct lone_lisp_value value);
enum lone_lisp_value_type lone_lisp_type_of(struct lone_lisp_value value);
struct lone_lisp_heap_value *lone_lisp_heap_value_of(struct lone_lisp *lone, struct lone_lisp_value value);
lone_lisp_integer lone_lisp_integer_of(struct lone_lisp_value value);

bool lone_lisp_is_register_value(struct lone_lisp_value value);
bool lone_lisp_is_heap_value(struct lone_lisp_value value);
bool lone_lisp_is_register_value_of_type(struct lone_lisp_value value, enum lone_lisp_value_type register_value_type);
bool lone_lisp_is_heap_value_of_type(struct lone_lisp *lone,
		struct lone_lisp_value value, enum lone_lisp_heap_value_type heap_value_type);

bool lone_lisp_is_module(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_function(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_primitive(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_applicable(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_list(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_list_or_nil(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_vector(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_table(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_has_bytes(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_bytes(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_text(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_symbol(struct lone_lisp *lone, struct lone_lisp_value value);

bool lone_lisp_is_nil(struct lone_lisp_value value);
bool lone_lisp_is_false(struct lone_lisp_value value);
bool lone_lisp_is_true(struct lone_lisp_value value);
bool lone_lisp_is_falsy(struct lone_lisp_value value);
bool lone_lisp_is_truthy(struct lone_lisp_value value);

bool lone_lisp_is_integer(struct lone_lisp *lone, struct lone_lisp_value value);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Comparison and equality functions.                                  │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
bool lone_lisp_has_same_type(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y);
bool lone_lisp_is_identical(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y);
bool lone_lisp_is_equivalent(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y);
bool lone_lisp_is_equal(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone integers are currently signed fixed-length integers.           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_integer_create(lone_lisp_integer integer);
struct lone_lisp_value lone_lisp_integer_from_pointer(void *pointer);

struct lone_lisp_value lone_lisp_integer_parse(struct lone_lisp *lone,
		unsigned char *digits, size_t count);

struct lone_lisp_value lone_lisp_zero(void);
struct lone_lisp_value lone_lisp_one(void);
struct lone_lisp_value lone_lisp_minus_one(void);

bool lone_lisp_integer_is_less_than(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y);
bool lone_lisp_integer_is_less_than_or_equal_to(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y);
bool lone_lisp_integer_is_greater_than(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y);
bool lone_lisp_integer_is_greater_than_or_equal_to(struct lone_lisp *lone, struct lone_lisp_value x, struct lone_lisp_value y);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    General constructor for lone heap values.                           │
   │    Meant for other constructors to use.                                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_value_from_heap_value(struct lone_lisp *lone, struct lone_lisp_heap_value *heap_value);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone modules are named isolated environments for evaluation.        │
   │                                                                        │
   │    The lisp interpreter contains a top level environment.              │
   │    This environment contains only the import/export primitives.        │
   │    New modules have clean environments which inherit from it.          │
   │    This allows complete control over the symbols and the namespace.    │
   │                                                                        │
   │    Each module corresponds roughly to one .ln file on disk.            │
   │    These module files are text files containing lone lisp code         │
   │    which may import or export symbols from or to other modules.        │
   │    The lone interpreter's import primitive will search for files       │
   │    to load in conventional locations, enabling library development.    │
   │                                                                        │
   │    A special nameless module known as the null module                  │
   │    contains code read in from standard input.                          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_module_create(struct lone_lisp *lone, struct lone_lisp_value name);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone functions represent a body of executable lone lisp code.       │
   │    They have a list of argument names to be bound during function      │
   │    application, a list of expressions to be evaluated when called      │
   │    and a closure: a reference to the environment it was defined in.    │
   │                                                                        │
   │    To apply a function is to create a new environment with its         │
   │    argument names bound to the given arguments and then evaluate       │
   │    the function's expressions in the context of that environment.      │
   │                                                                        │
   │    The function flags control how the function is applied.             │
   │    It may be configured to receive evaluated or unevaluated            │
   │    arguments as well as to evaluate the result automatically.          │
   │    These features allow code manipulation and generation.              │
   │    It may also be configured to be variadic: all arguments             │
   │    are collected into a list and passed as a single argument.          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_function_create(struct lone_lisp *lone,
		struct lone_lisp_value arguments, struct lone_lisp_value code,
		struct lone_lisp_value environment, struct lone_lisp_function_flags flags);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Primitives are lone functions implemented in C.                     │
   │    They are always variadic and must check their arguments.            │
   │    All of them must follow the primitive function prototype.           │
   │    They also have closures which are pointers to arbitrary data.       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_primitive_create(struct lone_lisp *lone, char *name,
		lone_lisp_primitive_function function, struct lone_lisp_value closure,
		struct lone_lisp_function_flags flags);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone continuations reify segments of the lisp machine stack         │
   │    into a callable that can be used to resume the computation          │
   │    by plugging the argument into it.                                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_continuation_create(struct lone_lisp *lone,
		size_t frame_count, struct lone_lisp_machine_stack_frame *frames);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone generators wrap around normal functions to give them           │
   │    their own stack, separate from the normal machine stack.            │
   │    This allows them to freely suspend execution and even               │
   │    yield values back to the caller.                                    │
   │                                                                        │
   │    Generators are semicoroutines, a specialized form of coroutine      │
   │    which can only yield control back to its calling subroutine.        │
   │    This restriction enables a more efficient implementation.           │
   │                                                                        │
   │    Delimited continuations are so flexible they could be used          │
   │    to implement generators. However, it's still desirable to           │
   │    have a proper type for this due to performance.                     │
   │    Continuations copy around stack frames.                             │
   │    Generators copy around stack pointers.                              │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_generator_create(struct lone_lisp *lone,
		struct lone_lisp_value function, size_t stack_size);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone lists are pairs of lone values: first and rest.                │
   │    Usually first is the element while rest is another pair.            │
   │    This explains their names. Also called car and cdr.                 │
   │                                                                        │
   │    Although often the case, rest need not be another pair.             │
   │    Any other object may be set: (1 . 2); first = 1, rest = 2.          │
   │    So rest could also be named second.                                 │
   │                                                                        │
   │    A list with null first and rest pointers is known as nil.           │
   │    It is provided as a constant by the lone interpreter.               │
   │    Their presence in the rest of a list marks its end.                 │
   │    New nil values may be created by C code that builds lists.          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_list_create(struct lone_lisp *lone,
		struct lone_lisp_value first, struct lone_lisp_value rest);
struct lone_lisp_value lone_lisp_list_create_nil(struct lone_lisp *lone);

bool lone_lisp_list_has_rest(struct lone_lisp *lone, struct lone_lisp_value value);
struct lone_lisp_value lone_lisp_list_first(struct lone_lisp *lone, struct lone_lisp_value value);
struct lone_lisp_value lone_lisp_list_rest(struct lone_lisp *lone, struct lone_lisp_value value);

struct lone_lisp_value lone_lisp_list_set_first(struct lone_lisp *lone,
		struct lone_lisp_value list, struct lone_lisp_value value);
struct lone_lisp_value lone_lisp_list_set_rest(struct lone_lisp *lone,
		struct lone_lisp_value list, struct lone_lisp_value rest);

struct lone_lisp_value lone_lisp_list_append(struct lone_lisp *lone,
		struct lone_lisp_value *first, struct lone_lisp_value *head,
		struct lone_lisp_value value);

bool lone_lisp_list_destructure(struct lone_lisp *lone, struct lone_lisp_value list, size_t count, ...);
struct lone_lisp_value lone_lisp_list_build(struct lone_lisp *lone, size_t count, ...);
struct lone_lisp_value lone_lisp_list_flatten(struct lone_lisp *lone, struct lone_lisp_value list);
struct lone_lisp_value lone_lisp_list_to_vector(struct lone_lisp *lone, struct lone_lisp_value list);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone vectors are simple dynamic arrays of lone values.              │
   │    They grow automatically as elements are added.                      │
   │    Any index may be used regardless of current length:                 │
   │    all the elements remain unset as the array grows.                   │
   │    The array is zero filled which makes unset elements nil.            │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_vector_create(struct lone_lisp *lone, size_t capacity);
size_t lone_lisp_vector_count(struct lone_lisp *lone, struct lone_lisp_value vector);
void lone_lisp_vector_resize(struct lone_lisp *lone, struct lone_lisp_value vector, size_t new_capacity);

struct lone_lisp_value lone_lisp_vector_get_value_at(struct lone_lisp *lone, struct lone_lisp_value vector, size_t i);
void lone_lisp_vector_set_value_at(struct lone_lisp *lone, struct lone_lisp_value vector, size_t i,
		struct lone_lisp_value value);

struct lone_lisp_value lone_lisp_vector_get(struct lone_lisp *lone, struct lone_lisp_value vector,
		struct lone_lisp_value index);
void lone_lisp_vector_set(struct lone_lisp *lone, struct lone_lisp_value vector,
		struct lone_lisp_value index, struct lone_lisp_value value);

bool lone_lisp_vector_contains(struct lone_lisp *lone, struct lone_lisp_value vector, struct lone_lisp_value value);

void lone_lisp_vector_push(struct lone_lisp *lone, struct lone_lisp_value vector, struct lone_lisp_value value);
void lone_lisp_vector_push_va_list(struct lone_lisp *lone, struct lone_lisp_value vector, size_t count, va_list arguments);
void lone_lisp_vector_push_all(struct lone_lisp *lone, struct lone_lisp_value vector, size_t count, ...);
struct lone_lisp_value lone_lisp_vector_build(struct lone_lisp *lone, size_t count, ...);

#define LONE_LISP_VECTOR_FOR_EACH(lone, entry, vector, i)                                \
	for ((i) = 0, (entry) = lone_lisp_vector_get_value_at((lone), (vector), 0);      \
	     (i) < lone_lisp_vector_count((lone), (vector));                             \
	     ++(i), (entry) = lone_lisp_vector_get_value_at((lone), (vector), (i)))

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Hash table functions.                                               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_table_create(struct lone_lisp *lone,
		size_t capacity, struct lone_lisp_value prototype);

struct lone_lisp_value lone_lisp_table_get(struct lone_lisp *lone, struct lone_lisp_value table,
		struct lone_lisp_value key);

void lone_lisp_table_set(struct lone_lisp *lone, struct lone_lisp_value table,
		struct lone_lisp_value key, struct lone_lisp_value value);

void lone_lisp_table_delete(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_lisp_value key);

size_t lone_lisp_table_count(struct lone_lisp *lone, struct lone_lisp_value table);
struct lone_lisp_value lone_lisp_table_key_at(struct lone_lisp *lone, struct lone_lisp_value table, lone_size i);
struct lone_lisp_value lone_lisp_table_value_at(struct lone_lisp *lone, struct lone_lisp_value table, lone_size i);

#define LONE_LISP_TABLE_FOR_EACH(lone, entry, table, i)                                            \
	for ((i) = 0, (entry) = &lone_lisp_heap_value_of((lone), (table))->as.table.entries[0];    \
	     (i) < lone_lisp_heap_value_of((lone), (table))->as.table.count;                       \
	     ++(i), (entry) = &lone_lisp_heap_value_of((lone), (table))->as.table.entries[(i)])

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone symbols are like lone texts but are interned in a table.       │
   │    Symbol table interning deduplicates them in memory,                 │
   │    enabling fast identity-based comparisons via pointer equality.      │
   │    However, this means they won't be garbage collected.                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_intern(struct lone_lisp *lone,
		unsigned char *bytes, size_t count, bool should_deallocate);

struct lone_lisp_value lone_lisp_intern_bytes(struct lone_lisp *lone,
		struct lone_bytes bytes, bool should_deallocate);

struct lone_lisp_value lone_lisp_intern_text(struct lone_lisp *lone,
		struct lone_lisp_value text);

struct lone_lisp_value lone_lisp_intern_c_string(struct lone_lisp *lone,
		char *c_string);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone texts are lone's strings and represent UTF-8 encoded text.     │
   │    Transfer and creation functions work like lone bytes.               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_text_transfer(struct lone_lisp *lone,
		unsigned char *text, size_t length, bool should_deallocate);

struct lone_lisp_value lone_lisp_text_transfer_bytes(struct lone_lisp *lone,
		struct lone_bytes bytes, bool should_deallocate);

struct lone_lisp_value lone_lisp_text_copy(struct lone_lisp *lone,
		unsigned char *text, size_t length);

struct lone_lisp_value lone_lisp_text_from_c_string(struct lone_lisp *lone,
		char *c_string);

struct lone_lisp_value lone_lisp_text_to_symbol(struct lone_lisp *lone,
		struct lone_lisp_value text);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone bytes values can be created zero-filled for a given size       │
   │    or they can be initialized with a pointer to a memory block         │
   │    of known size.                                                      │
   │                                                                        │
   │    They can take ownership of arbitrary memory blocks via transfers    │
   │    or make copies of their input data.                                 │
   │                                                                        │
   │    Transferring memory blocks allows control over deallocation.        │
   │    Disabling deallocation on garbage collection allows pointing to     │
   │    data such as statically allocated buffers and C string literals.    │
   │    Enabling deallocation will cause the pointer to be deallocated      │
   │    when the bytes object is garbage collected. Two bytes objects       │
   │    cannot own the same memory block; it would lead to double free.     │
   │    This mode of operation is suitable for memory allocated by lone.    │
   │                                                                        │
   │    Copies will automatically include a hidden trailing null            │
   │    byte to ease compatibility with code expecting C strings.           │
   │    It's impossible to escape from them since system calls use them.    │
   │    Transferred buffers should also contain that null byte              │
   │    but the lone bytes type currently has no way to enforce this.       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_bytes_transfer(struct lone_lisp *lone,
		unsigned char *pointer, size_t count, bool should_deallocate);

struct lone_lisp_value lone_lisp_bytes_transfer_bytes(struct lone_lisp *lone,
		struct lone_bytes bytes, bool should_deallocate);

struct lone_lisp_value lone_lisp_bytes_copy(struct lone_lisp *lone,
		unsigned char *pointer, size_t count);

struct lone_lisp_value lone_lisp_bytes_create(struct lone_lisp *lone,
		size_t count);

/* ╭───────────────────────┨ LONE LISP INTERPRETER ┠────────────────────────╮
   │                                                                        │
   │    The lone lisp interpreter is composed of all internal state         │
   │    necessary to process useful programs. It includes memory,           │
   │    references to all allocated objects, a table of interned            │
   │    symbols, references to constant values such as nil and              │
   │    a table of loaded modules and the top level null module.            │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

enum lone_lisp_machine_step {
	LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION,
	LONE_LISP_MACHINE_STEP_EVALUATED_OPERATOR,
	LONE_LISP_MACHINE_STEP_OPERAND_EVALUATION,
	LONE_LISP_MACHINE_STEP_OPERAND_ACCUMULATION,
	LONE_LISP_MACHINE_STEP_LAST_OPERAND_ACCUMULATION,
	LONE_LISP_MACHINE_STEP_APPLICATION,
	LONE_LISP_MACHINE_STEP_AFTER_APPLICATION,
	LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION,
	LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_NEXT,
	LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_FROM_PRIMITIVE,
	LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_FROM_PRIMITIVE_NEXT,
	LONE_LISP_MACHINE_STEP_RESUME_PRIMITIVE,
	LONE_LISP_MACHINE_STEP_GENERATOR_RETURN,
	LONE_LISP_MACHINE_STEP_HALT,
};

enum lone_lisp_machine_stack_frame_type {
	LONE_LISP_MACHINE_STACK_FRAME_TYPE_VALUE,
	LONE_LISP_MACHINE_STACK_FRAME_TYPE_INTEGER,
	LONE_LISP_MACHINE_STACK_FRAME_TYPE_STEP,
	LONE_LISP_MACHINE_STACK_FRAME_TYPE_PRIMITIVE_STEP,
	LONE_LISP_MACHINE_STACK_FRAME_TYPE_FUNCTION_DELIMITER,
	LONE_LISP_MACHINE_STACK_FRAME_TYPE_CONTINUATION_DELIMITER,
	LONE_LISP_MACHINE_STACK_FRAME_TYPE_GENERATOR_DELIMITER,
};

struct lone_lisp_machine_stack_frame {
	enum lone_lisp_machine_stack_frame_type type;
	union {
		struct lone_lisp_value value;
		lone_lisp_integer integer;
		enum lone_lisp_machine_step step;
		lone_lisp_integer primitive_step;
	} as;
};

struct lone_lisp_machine {
	/* the machine's stack, where its registers and return steps are saved */
	struct lone_lisp_machine_stack stack;

	/* where in the machine to return to after a computation step is done */
	enum lone_lisp_machine_step step;

	struct {
		/* where to return in a primitive after a computation step is done */
		long step;
		struct lone_lisp_value closure;
	} primitive;

	struct lone_lisp_value module;      /* the module where evaluation is occurring */
	struct lone_lisp_value expression;  /* expression to evaluate */
	struct lone_lisp_value environment; /* environment to evaluate expression in */
	struct lone_lisp_value value;       /* result of the last evaluation */
	struct lone_lisp_value applicable;  /* value to which arguments will be applied */
	struct lone_lisp_value list;        /* accumulated results of evaluation of multiple expressions */
	struct lone_lisp_value unevaluated; /* remaining expressions queued for evaluation */
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
	size_t capacity;
	size_t count;
	struct lone_lisp_heap_value *values;
};

struct lone_lisp {
	struct lone_system *system;
	void *native_stack;
	struct lone_lisp_heap heap;
	struct lone_lisp_value symbol_table;
	struct {
		struct lone_lisp_value loaded;
		struct lone_lisp_value embedded;
		struct lone_lisp_value null;
		struct lone_lisp_value top_level_environment;
		struct lone_lisp_value path;
	} modules;
};

#endif /* LONE_LISP_TYPES_HEADER */
