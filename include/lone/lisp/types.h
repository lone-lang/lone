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

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Tag byte values for tagged value words.                             │
   │                                                                        │
   │    Bit 0 discriminates heap (0) from non-heap (1) values.              │
   │    For heap values, bits 1-4 encode the type (16 slots, 11 used).      │
   │    For non-heap values, the remaining bits distinguish type.           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

enum lone_lisp_tag {

	/* heap value tags: bit 0 = 0, type in bits 1-4 */
	LONE_LISP_TAG_MODULE       = 0x00,
	LONE_LISP_TAG_FUNCTION     = 0x02,
	LONE_LISP_TAG_PRIMITIVE    = 0x04,
	LONE_LISP_TAG_CONTINUATION = 0x06,
	LONE_LISP_TAG_GENERATOR    = 0x08,
	LONE_LISP_TAG_LIST         = 0x0A,
	LONE_LISP_TAG_VECTOR       = 0x0C,
	LONE_LISP_TAG_TABLE        = 0x0E,
	LONE_LISP_TAG_SYMBOL       = 0x10,
	LONE_LISP_TAG_TEXT         = 0x12,
	LONE_LISP_TAG_BYTES        = 0x14,

	/* non-heap value tags: bit 0 = 1 */
	LONE_LISP_TAG_INTEGER = 0x01,
	LONE_LISP_TAG_NIL     = 0x03,
	LONE_LISP_TAG_TRUE    = 0x05,
	LONE_LISP_TAG_FALSE   = 0x07,

	/* Inline symbol tags:
	 *
	 * 	bit 7 = 1
	 * 	bits 4-6 = 000
	 * 	bits 1-3 = length
	 * 	bit 0 = 1
	 *
	 * Symbols with names ≤ 7 bytes are encoded directly
	 * in the word. Bytes are stored in the data bits
	 * starting at bit 8. Two inline symbols with identical
	 * names produce identical words.
	 */
	LONE_LISP_TAG_INLINE_SYMBOL_0 = 0x81, /* length 0 */
	LONE_LISP_TAG_INLINE_SYMBOL_1 = 0x83, /* length 1 */
	LONE_LISP_TAG_INLINE_SYMBOL_2 = 0x85, /* length 2 */
	LONE_LISP_TAG_INLINE_SYMBOL_3 = 0x87, /* length 3 */
	LONE_LISP_TAG_INLINE_SYMBOL_4 = 0x89, /* length 4 */
	LONE_LISP_TAG_INLINE_SYMBOL_5 = 0x8B, /* length 5 */
	LONE_LISP_TAG_INLINE_SYMBOL_6 = 0x8D, /* length 6 */
	LONE_LISP_TAG_INLINE_SYMBOL_7 = 0x8F, /* length 7 */

	/* Inline text tags:
	 *
	 * 	bit 7 = 1
	 * 	bits 4-6 = 001
	 * 	bits 1-3 = length
	 * 	bit 0 = 1
	 *
	 * Texts with content ≤ 7 bytes are encoded directly
	 * in the word. Bytes are stored in the data bits
	 * starting at bit 8. Two inline texts with identical
	 * content produce identical words.
	 */
	LONE_LISP_TAG_INLINE_TEXT_0 = 0x91, /* length 0 */
	LONE_LISP_TAG_INLINE_TEXT_1 = 0x93, /* length 1 */
	LONE_LISP_TAG_INLINE_TEXT_2 = 0x95, /* length 2 */
	LONE_LISP_TAG_INLINE_TEXT_3 = 0x97, /* length 3 */
	LONE_LISP_TAG_INLINE_TEXT_4 = 0x99, /* length 4 */
	LONE_LISP_TAG_INLINE_TEXT_5 = 0x9B, /* length 5 */
	LONE_LISP_TAG_INLINE_TEXT_6 = 0x9D, /* length 6 */
	LONE_LISP_TAG_INLINE_TEXT_7 = 0x9F, /* length 7 */

	/* Inline bytes tags:
	 *
	 * 	bit 7 = 1
	 * 	bits 4-6 = 010
	 * 	bits 1-3 = length
	 * 	bit 0 = 1
	 *
	 * Byte sequences with ≤ 7 bytes are encoded directly
	 * in the word. Bytes are stored in the data bits
	 * starting at bit 8. Two inline byte values with
	 * identical content produce identical words.
	 */
	LONE_LISP_TAG_INLINE_BYTES_0 = 0xA1, /* length 0 */
	LONE_LISP_TAG_INLINE_BYTES_1 = 0xA3, /* length 1 */
	LONE_LISP_TAG_INLINE_BYTES_2 = 0xA5, /* length 2 */
	LONE_LISP_TAG_INLINE_BYTES_3 = 0xA7, /* length 3 */
	LONE_LISP_TAG_INLINE_BYTES_4 = 0xA9, /* length 4 */
	LONE_LISP_TAG_INLINE_BYTES_5 = 0xAB, /* length 5 */
	LONE_LISP_TAG_INLINE_BYTES_6 = 0xAD, /* length 6 */
	LONE_LISP_TAG_INLINE_BYTES_7 = 0xAF, /* length 7 */

	/* Lone lisp machine stack frame tags
	 *
	 * Stack frames are 8-byte tagged words
	 * sharing the same format as lone_lisp_value.
	 * Lisp values are stored directly.
	 * Steps and delimiters use dedicated tags
	 * following the same even/odd scheme
	 * for the garbage collector's benefit:
	 * bit 0 = 1 means no heap reference,
	 * bit 0 = 0 means index in the heap.
	 * The garbage collector will skip
	 * odd values and trace even values.
	 *
	 * Even-tagged frame types store heap
	 * references that the GC must trace:
	 * function delimiters store the function
	 * being applied and generator delimiters
	 * store the generator itself.
	 */
	LONE_LISP_TAG_STEP                   = 0x21,
	LONE_LISP_TAG_PRIMITIVE_STEP         = 0x23,
	LONE_LISP_TAG_FUNCTION_DELIMITER     = 0x40,
	LONE_LISP_TAG_CONTINUATION_DELIMITER = 0x43,
	LONE_LISP_TAG_INTERCEPTOR_DELIMITER  = 0x45,
	LONE_LISP_TAG_GENERATOR_DELIMITER    = 0x60,

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

struct lone_lisp_symbol {
	struct lone_bytes name;
	unsigned long hash;
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

struct lone_lisp_table_entry {
	struct lone_lisp_value key;
	struct lone_lisp_value value;
};

struct lone_lisp_table {
	size_t count;
	size_t capacity;
	size_t *indexes;
	struct lone_lisp_table_entry *entries;
	struct lone_lisp_value prototype;
};

struct lone_lisp_heap_value {
	struct {
		bool should_deallocate_bytes: 1;
	};

	enum lone_lisp_tag type; /* tag byte, set at allocation for GC sweep */

	union {
		struct lone_lisp_module module;
		struct lone_lisp_function function;
		struct lone_lisp_primitive primitive;
		struct lone_lisp_continuation continuation;
		struct lone_lisp_generator generator;
		struct lone_lisp_list list;
		struct lone_lisp_vector vector;
		struct lone_lisp_table table;
		struct lone_lisp_symbol symbol;
		struct lone_bytes bytes; /* used by texts and bytes values */

		struct {
			long forwarding_index;
		} metadata;
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

enum lone_lisp_tag lone_lisp_type_of(struct lone_lisp_value value);
struct lone_lisp_heap_value *lone_lisp_heap_value_of(struct lone_lisp *lone, struct lone_lisp_value value);
lone_lisp_integer lone_lisp_integer_of(struct lone_lisp_value value);
struct lone_lisp_value lone_lisp_retag(struct lone_lisp_value value, enum lone_lisp_tag new_tag);
struct lone_lisp_value lone_lisp_retag_frame(struct lone_lisp_machine_stack_frame frame, enum lone_lisp_tag new_tag);

bool lone_lisp_is_register_value(struct lone_lisp_value value);
bool lone_lisp_is_heap_value(struct lone_lisp_value value);

bool lone_lisp_is_module(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_function(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_primitive(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_applicable(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_list(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_vector(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_table(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_has_bytes(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_bytes(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_text(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_symbol(struct lone_lisp *lone, struct lone_lisp_value value);
bool lone_lisp_is_inline_symbol(struct lone_lisp_value value);
bool lone_lisp_is_inline_text(struct lone_lisp_value value);
bool lone_lisp_is_inline_bytes(struct lone_lisp_value value);
bool lone_lisp_is_inline_value(struct lone_lisp_value value);

/* Extract bytes from any inline value (symbol, text, or bytes).
 * The returned pointer points into the tagged word at the address
 * of the value parameter, so the caller must keep the value alive
 * for the lifetime of the returned struct lone_bytes.
 */
struct lone_bytes lone_lisp_inline_value_bytes(struct lone_lisp_value *value);

/* Extract symbol name bytes from either heap or inline symbols.
 * For inline symbols, the returned pointer points into the tagged word
 * at the address of the value parameter, so the caller must keep
 * the value alive for the lifetime of the returned struct lone_bytes.
 */
struct lone_bytes lone_lisp_symbol_name(struct lone_lisp *lone, struct lone_lisp_value *value);

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

struct lone_lisp_value lone_lisp_value_from_heap_value(struct lone_lisp *lone,
		struct lone_lisp_heap_value *heap_value, enum lone_lisp_tag tag);

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

struct lone_lisp_value lone_lisp_table_get_by_symbol(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_bytes bytes);
struct lone_lisp_value lone_lisp_table_get_by_text(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_bytes bytes);
struct lone_lisp_value lone_lisp_table_get_by_bytes(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_bytes bytes);

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

struct lone_lisp_value lone_lisp_inline_symbol_create(unsigned char *bytes, size_t count);
struct lone_lisp_value lone_lisp_inline_text_create(unsigned char *bytes, size_t count);
struct lone_lisp_value lone_lisp_inline_bytes_create(unsigned char *bytes, size_t count);

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
	LONE_LISP_MACHINE_STEP_EVALUATE,
	LONE_LISP_MACHINE_STEP_APPLY,
	LONE_LISP_MACHINE_STEP_EVALUATED_OPERATOR,
	LONE_LISP_MACHINE_STEP_OPERAND_EVALUATION,
	LONE_LISP_MACHINE_STEP_OPERAND_ACCUMULATION,
	LONE_LISP_MACHINE_STEP_LAST_OPERAND_ACCUMULATION,
	LONE_LISP_MACHINE_STEP_APPLICATION,
	LONE_LISP_MACHINE_STEP_AFTER_APPLICATION,
	LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION,
	LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_NEXT,
	LONE_LISP_MACHINE_STEP_RESUME_PRIMITIVE,
	LONE_LISP_MACHINE_STEP_TAIL_RETURN,
	LONE_LISP_MACHINE_STEP_GENERATOR_RETURN,
	LONE_LISP_MACHINE_STEP_HALT,
};

struct lone_lisp_machine_stack_frame {
	long tagged;
};

struct lone_lisp_machine {
	/* the machine's stack, where its registers and return steps are saved */
	struct lone_lisp_machine_stack stack;
	/* initial stack frame count, used by shrink-at-reset */
	size_t initial_stack_count;

	/* where in the machine to return to after a computation step is done */
	enum lone_lisp_machine_step step;

	struct {
		/* where to return in a primitive after a computation step is done */
		long step;
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
   │    Lone lisp values are allocated from a flat and contiguous heap.     │
   │    This heap is a single mmap'd array of lone_lisp_heap_value          │
   │    structures, indexed by position. Tagged lone_lisp_value words       │
   │    encode a shifted index into this array rather than a pointer.       │
   │    By making lisp values position independent, memory management       │
   │    is greatly simplified to the point efficient reallocation is        │
   │    provided by Linux itself via mremap.                                │
   │                                                                        │
   │    Three separate bitmaps track per-value metadata:                    │
   │                                                                        │
   │      ◦ live   - whether the value is allocated and in use              │
   │      ◦ marked - whether the value is reachable                         │
   │      ◦ pinned - whether the value cannot move during compaction        │
   │                                                                        │
   │    The bitmaps are word-aligned mmap'd pages that grow                 │
   │    in tandem with the values array via mremap.                         │
   │                                                                        │
   │    first_dead is the lowest value index thought to be dead.            │
   │    Allocation scans the live bitmap starting from first_dead           │
   │    looking for the index of the first zero bit. When found,            │
   │    the dead value at the corresponding index is allocated.             │
   │    When not found, heap capacity grows via mremap with MAYMOVE.        │
   │                                                                        │
   │    Garbage collection proceeds in three phases:                        │
   │                                                                        │
   │      ◦ Mark    - walks the object graph starting from known roots:     │
   │                  lisp stack, generator stacks, native stack,           │
   │                  native registers, lisp registers, internal values;    │
   │                  sets the mark bit for every reachable value           │
   │      ◦ Sweep   - deallocates resources owned by unmarked values,       │
   │                  clears their live bits and recalculates heap bounds   │
   │      ◦ Compact - moves unpinned live values downward into the heap,    │
   │                  records forwarding indices where they used to be      │
   │                  and rewrites all value references across the entire   │
   │                  interpreter to reflect their new positions            │
   │                                                                        │
   │    The conservative native stack scanner recognizes both raw           │
   │    pointers into the heap values array and tagged index values.        │
   │    The precise lisp stack scanner uses typed frames to avoid           │
   │    false positives.                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_heap {
	size_t capacity;
	size_t count;
	size_t first_dead;
	struct lone_lisp_heap_value *values;

	struct {
		void *live;
		void *marked;
		void *pinned;
	} bits;
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
