/* SPDX-License-Identifier: AGPL-3.0-or-later */

/* ╭─────────────────────────────┨ LONE LISP ┠──────────────────────────────╮
   │                                                                        │
   │                       The standalone Linux Lisp                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#include <stdbool.h>
#include <stdarg.h>

#include <linux/types.h>
#include <linux/auxvec.h>
#include <linux/unistd.h>
#include <linux/errno.h>
#include <linux/fcntl.h>

typedef __kernel_size_t size_t;
typedef __kernel_ssize_t ssize_t;

#include LONE_ARCH_SOURCE

static void __attribute__((noreturn)) linux_exit(int code)
{
	system_call_1(__NR_exit, code);
	__builtin_unreachable();
}

static long __attribute__((tainted_args)) linux_openat(int dirfd, unsigned char *path, int flags)
{
	return system_call_4(__NR_openat, dirfd, (long) path, flags, 0);
}

static long __attribute__((tainted_args)) linux_close(int fd)
{
	return system_call_1(__NR_close, fd);
}

static ssize_t __attribute__((fd_arg_read(1), tainted_args)) linux_read(int fd, const void *buffer, size_t count)
{
	return system_call_3(__NR_read, fd, (long) buffer, (long) count);
}

static ssize_t __attribute__((fd_arg_write(1), tainted_args)) linux_write(int fd, const void *buffer, size_t count)
{
	return system_call_3(__NR_write, fd, (long) buffer, (long) count);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │                                      bits = 32    |    bits = 64       │
   │    digits = ceil(bits * log10(2)) =  10           |    20              │
   │                                                                        │
   │    https://en.wikipedia.org/wiki/FNV_hash                              │
   │    https://datatracker.ietf.org/doc/draft-eastlake-fnv/                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#if __BITS_PER_LONG == 64
	#define DECIMAL_DIGITS_PER_LONG 20
	#define FNV_PRIME 0x00000100000001B3UL
	#define FNV_OFFSET_BASIS 0xCBF29CE484222325UL
#elif __BITS_PER_LONG == 32
	#define DECIMAL_DIGITS_PER_LONG 10
	#define FNV_PRIME 0x01000193UL
	#define FNV_OFFSET_BASIS 0x811C9DC5
#else
	#error "Unsupported architecture"
#endif

#ifndef LONE_BUFFER_SIZE
	#define LONE_BUFFER_SIZE 4096
#endif

#ifndef LONE_MEMORY_SIZE
	#define LONE_MEMORY_SIZE (1024 * 1024)
#endif

#ifndef LONE_ALIGNMENT
	#define LONE_ALIGNMENT 16
#endif

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
   │        ◦ Integer      the signed integer type                          │
   │        ◦ Pointer      the memory addressing and dereferencing type     │
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
	LONE_INTEGER,
	LONE_POINTER,
};

struct lone_bytes {
	size_t count;
	unsigned char *pointer;
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

struct lone_lisp;
typedef struct lone_value *(*lone_primitive)(struct lone_lisp *lone,
                                             struct lone_value *closure,
                                             struct lone_value *environment,
                                             struct lone_value *arguments);

struct lone_primitive {
	struct lone_value *name;
	lone_primitive function;
	struct lone_value *closure;
	struct lone_function_flags flags;    /* primitives always accept variable arguments */
};

struct lone_module {
	struct lone_value *name;
	struct lone_value *environment;
};

struct lone_value {
	enum lone_type type;
	union {
		struct lone_module module;
		struct lone_function function;
		struct lone_primitive primitive;
		struct lone_list list;
		struct lone_vector vector;
		struct lone_table table;
		struct lone_bytes bytes;   /* also used by texts and symbols */
		long integer;
		void *pointer;
	};
};

typedef bool (*lone_predicate)(struct lone_value *);
typedef bool (*lone_comparator)(struct lone_value *, struct lone_value *);

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
		struct lone_value *import;
		struct lone_value *path;
	} modules;
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

struct lone_value_header {
	bool live: 1;
	bool marked: 1;
};

struct lone_value_container {
	struct lone_value_header header;
	struct lone_value value;
};

struct lone_heap {
	struct lone_heap *next;
	size_t count;
	struct lone_value_container values[];
};

static void lone_memory_move(void *from, void *to, size_t count)
{
	unsigned char *source = from, *destination = to;

	if (source >= destination) {
		/* destination is at or behind source, copy forwards */
		while (count--) { *destination++ = *source++; }
	} else {
		/* destination is ahead of source, copy backwards */
		source += count; destination += count;
		while (count--) { *--destination = *--source; }
	}
}

static void lone_memory_split(struct lone_memory *block, size_t used)
{
	size_t excess = block->size - used;

	/* split block if there's enough space to allocate at least 1 byte */
	if (excess >= sizeof(struct lone_memory) + 1) {
		struct lone_memory *new = (struct lone_memory *) __builtin_assume_aligned(block->pointer + used, LONE_ALIGNMENT);
		new->next = block->next;
		new->prev = block;
		new->free = 1;
		new->size = excess - sizeof(struct lone_memory);
		block->next = new;
		block->size -= excess + sizeof(struct lone_memory);
	}
}

static void lone_memory_coalesce(struct lone_memory *block)
{
	struct lone_memory *next;

	if (block && block->free) {
		next = block->next;
		if (next && next->free) {
			block->size += next->size + sizeof(struct lone_memory);
			next = block->next = next->next;
			if (next) { next->prev = block; }
		}
	}
}

static size_t __attribute__((const)) lone_next_power_of_2(size_t n)
{
	size_t next = 1;
	while (next < n) { next *= 2; }
	return next;
}

static size_t __attribute__((const)) lone_next_power_of_2_multiple(size_t n, size_t m)
{
	m = lone_next_power_of_2(m);
	return (n + m - 1) & (~(m - 1));
}

static size_t __attribute__((const)) lone_align(size_t size, size_t alignment)
{
	return lone_next_power_of_2_multiple(size, alignment);
}

static void * __attribute__((malloc, alloc_size(2), alloc_align(3))) lone_allocate_aligned(struct lone_lisp *lone, size_t requested_size, size_t alignment)
{
	size_t needed_size = requested_size + sizeof(struct lone_memory);
	struct lone_memory *block;

	needed_size = lone_align(needed_size, alignment);

	for (block = lone->memory.general; block; block = block->next) {
		if (block->free && block->size >= needed_size)
			break;
	}

	if (!block) { linux_exit(-1); }

	block->free = 0;
	lone_memory_split(block, needed_size);

	return block->pointer;
}

static void * __attribute__((malloc, alloc_size(2), assume_aligned(LONE_ALIGNMENT))) lone_allocate(struct lone_lisp *lone, size_t requested_size)
{
	return lone_allocate_aligned(lone, requested_size, LONE_ALIGNMENT);
}

static void lone_deallocate(struct lone_lisp *lone, void * pointer)
{
	struct lone_memory *block = ((struct lone_memory *) pointer) - 1;
	block->free = 1;

	lone_memory_coalesce(block);
	lone_memory_coalesce(block->prev);
}

static void * __attribute__((alloc_size(3))) lone_reallocate(struct lone_lisp *lone, void *pointer, size_t size)
{
	struct lone_memory *old = ((struct lone_memory *) pointer) - 1,
	                   *new = ((struct lone_memory *) lone_allocate(lone, size)) - 1;

	if (pointer) {
		lone_memory_move(old->pointer, new->pointer, new->size < old->size ? new->size : old->size);
		lone_deallocate(lone, pointer);
	}

	return new->pointer;
}

static struct lone_heap *lone_allocate_heap(struct lone_lisp *lone, size_t count)
{
	size_t i, size = sizeof(struct lone_heap) + (sizeof(struct lone_value_container) * count);
	struct lone_heap *heap = lone_allocate(lone, size);
	heap->next = 0;
	heap->count = count;
	for (i = 0; i < count; ++i) {
		heap->values[i].header.live = false;
		heap->values[i].header.marked = false;
	}
	return heap;
}

static struct lone_value_container *lone_allocate_from_heap(struct lone_lisp *lone)
{
	struct lone_value_container *element;
	struct lone_heap *heap, *prev;
	size_t i;

	for (prev = lone->memory.heaps, heap = prev; heap; prev = heap, heap = heap->next) {
		for (i = 0; i < heap->count; ++i) {
			element = &heap->values[i];

			if (!element->header.live) {
				goto resurrect;
			}
		}
	}

	heap = lone_allocate_heap(lone, lone->memory.heaps[0].count);
	prev->next = heap;
	element = &heap->values[0];

resurrect:
	element->header.live = true;
	return element;
}

static void lone_deallocate_dead_heaps(struct lone_lisp *lone)
{
	struct lone_heap *prev = lone->memory.heaps, *heap = prev->next;
	size_t i;

	while (heap) {
		for (i = 0; i < heap->count; ++i) {
			if (heap->values[i].header.live) { /* at least one live object */ goto next_heap; }
		}

		/* no live objects */
		prev->next = heap->next;
		lone_deallocate(lone, heap);
		heap = prev->next;
		continue;
next_heap:
		prev = heap;
		heap = heap->next;
	}
}

static inline struct lone_value_container *lone_value_to_container(struct lone_value* value)
{
	unsigned char *bytes;
	if (!value) { return 0; }
	bytes = (unsigned char *) __builtin_assume_aligned(value, __alignof__(struct lone_value));
	bytes -= __builtin_offsetof(struct lone_value_container, value);
	return (struct lone_value_container *) __builtin_assume_aligned(bytes, __alignof__(struct lone_value_container));
}

static void lone_mark_value(struct lone_value *value)
{
	struct lone_value_container *container = lone_value_to_container(value);

	if (!container || !container->header.live || container->header.marked) { return; }

	container->header.marked = true;

	switch (container->value.type) {
	case LONE_MODULE:
		lone_mark_value(container->value.module.name);
		lone_mark_value(container->value.module.environment);
		break;
	case LONE_FUNCTION:
		lone_mark_value(container->value.function.arguments);
		lone_mark_value(container->value.function.code);
		lone_mark_value(container->value.function.environment);
		break;
	case LONE_PRIMITIVE:
		lone_mark_value(container->value.primitive.name);
		lone_mark_value(container->value.primitive.closure);
		break;
	case LONE_LIST:
		lone_mark_value(container->value.list.first);
		lone_mark_value(container->value.list.rest);
		break;
	case LONE_VECTOR:
		for (size_t i = 0; i < container->value.vector.count; ++i) {
			lone_mark_value(container->value.vector.values[i]);
		}
		break;
	case LONE_TABLE:
		lone_mark_value(container->value.table.prototype);
		for (size_t i = 0; i < container->value.table.capacity; ++i) {
			lone_mark_value(container->value.table.entries[i].key);
			lone_mark_value(container->value.table.entries[i].value);
		}
		break;
	case LONE_SYMBOL:
	case LONE_TEXT:
	case LONE_BYTES:
	case LONE_POINTER:
	case LONE_INTEGER:
		/* these types do not contain any other values to mark */
		break;
	}
}

static void lone_mark_known_roots(struct lone_lisp *lone)
{
	lone_mark_value(lone->symbol_table);
	lone_mark_value(lone->constants.nil);
	lone_mark_value(lone->constants.truth);
	lone_mark_value(lone->modules.loaded);
	lone_mark_value(lone->modules.null);
	lone_mark_value(lone->modules.import);
	lone_mark_value(lone->modules.path);
}

static bool lone_points_within_range(void *pointer, void *start, void *end)
{
	return start <= pointer && pointer < end;
}

static bool lone_points_to_general_memory(struct lone_lisp *lone, void *pointer)
{
	struct lone_memory *general = lone->memory.general;
	return lone_points_within_range(pointer, general->pointer, general->pointer + general->size);
}

static bool lone_points_to_heap(struct lone_lisp *lone, void *pointer)
{
	struct lone_heap *heap;

	if (!lone_points_to_general_memory(lone, pointer)) { return false; }

	for (heap = lone->memory.heaps; heap; heap = heap->next) {
		if (lone_points_within_range(pointer, heap->values, heap->values + heap->count)) { return true; }
	}

	return false;
}

static void lone_find_and_mark_stack_roots(struct lone_lisp *lone)
{
	void *bottom = lone->memory.stack, *top = __builtin_frame_address(0), *tmp;
	void **pointer;

	if (top < bottom) {
		tmp = bottom;
		bottom = top;
		top = tmp;
	}

	pointer = bottom;

	while (pointer++ < top) {
		if (lone_points_to_heap(lone, *pointer)) {
			lone_mark_value(*pointer);
		}
	}
}

static void lone_mark_all_reachable_values(struct lone_lisp *lone)
{
	lone_registers registers;                /* stack space for registers */
	lone_save_registers(registers);          /* spill registers on stack */

	lone_mark_known_roots(lone);             /* precise */
	lone_find_and_mark_stack_roots(lone);    /* conservative */
}

static void lone_kill_all_unmarked_values(struct lone_lisp *lone)
{
	struct lone_value_container *value;
	struct lone_heap *heap;
	size_t i;

	for (heap = lone->memory.heaps; heap; heap = heap->next) {
		for (i = 0; i < heap->count; ++i) {
			value = &heap->values[i];

			if (!value->header.live) { continue; }

			if (!value->header.marked) {
				switch (value->value.type) {
				case LONE_BYTES:
				case LONE_TEXT:
				case LONE_SYMBOL:
					lone_deallocate(lone, value->value.bytes.pointer);
					break;
				case LONE_VECTOR:
					lone_deallocate(lone, value->value.vector.values);
					break;
				case LONE_TABLE:
					lone_deallocate(lone, value->value.table.entries);
					break;
				case LONE_MODULE:
				case LONE_FUNCTION:
				case LONE_PRIMITIVE:
				case LONE_LIST:
				case LONE_INTEGER:
				case LONE_POINTER:
					/* these types do not own any additional memory */
					break;
				}

				value->header.live = false;
				value->header.marked = false;
			}
		}
	}
}

static void lone_garbage_collector(struct lone_lisp *lone)
{
	lone_mark_all_reachable_values(lone);
	lone_kill_all_unmarked_values(lone);
	lone_deallocate_dead_heaps(lone);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Initializers and creation functions for lone's types.               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_value_create(struct lone_lisp *lone)
{
	struct lone_value_container *container = lone_allocate_from_heap(lone);
	return &container->value;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone bytes values are initialized with a pointer to a memory        │
   │    block of known size. It can take ownership of the block             │
   │    through the transfer functions or it can make a copy of it          │
   │    via the create functions. Ownership means the block is              │
   │    deallocated when the value is garbage collected,                    │
   │    so it is advisable to copy data not owned by lone                   │
   │    such as C string literals.                                          │
   │                                                                        │
   │    Copies will automatically include a hidden trailing null            │
   │    byte to ease compatibility with code expecting C strings.           │
   │    Transferred buffers should also contain that null byte              │
   │    but the lone bytes type currently has no way to enforce this.       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_bytes_transfer(struct lone_lisp *lone, unsigned char *pointer, size_t count)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_BYTES;
	value->bytes.count = count;
	value->bytes.pointer = pointer;
	return value;
}

static struct lone_value *lone_bytes_transfer_bytes(struct lone_lisp *lone, struct lone_bytes bytes)
{
	return lone_bytes_transfer(lone, bytes.pointer, bytes.count);
}

static struct lone_value *lone_bytes_create(struct lone_lisp *lone, unsigned char *pointer, size_t count)
{
	unsigned char *copy = lone_allocate(lone, count + 1);
	lone_memory_move(pointer, copy, count);
	copy[count] = '\0';
	return lone_bytes_transfer(lone, copy, count);
}

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
static struct lone_value *lone_list_create(struct lone_lisp *lone, struct lone_value *first, struct lone_value *rest)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_LIST;
	value->list.first = first;
	value->list.rest = rest;
	return value;
}

static struct lone_value *lone_list_create_nil(struct lone_lisp *lone)
{
	return lone_list_create(lone, 0, 0);
}

static struct lone_value *lone_nil(struct lone_lisp *lone)
{
	return lone->constants.nil;
}

static struct lone_value *lone_true(struct lone_lisp *lone)
{
	return lone->constants.truth;
}

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
   │    Primitives are lone functions implemented in C.                     │
   │    They are always variadic and must check their arguments.            │
   │    All of them must follow the primitive function prototype.           │
   │    They also have closures which are pointers to arbitrary data.       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_function_create(struct lone_lisp *lone, struct lone_value *arguments, struct lone_value *code, struct lone_value *environment, struct lone_function_flags flags)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_FUNCTION;
	value->function.arguments = arguments;
	value->function.code = code;
	value->function.environment = environment;
	value->function.flags = flags;
	return value;
}

static struct lone_value *lone_intern_c_string(struct lone_lisp *, char *);

static struct lone_value *lone_primitive_create(struct lone_lisp *lone, char *name, lone_primitive function, struct lone_value *closure, struct lone_function_flags flags)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_PRIMITIVE;
	value->primitive.name = lone_intern_c_string(lone, name);
	value->primitive.function = function;
	value->primitive.closure = closure;
	value->primitive.flags = flags;
	value->primitive.flags.variable_arguments = 1;
	return value;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone vectors are simple dynamic arrays of lone values.              │
   │    They grow automatically as elements are added.                      │
   │    Any index may be used regardless of current length:                 │
   │    all the elements remain unset as the array grows.                   │
   │    Unset elements are null pointers which are currently                │
   │    converted to nil automatically but might one day serve              │
   │    as an undefined value like in other languages.                      │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_vector_create(struct lone_lisp *lone, size_t capacity)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_VECTOR;
	value->vector.capacity = capacity;
	value->vector.count = 0;
	value->vector.values = lone_allocate(lone, capacity * sizeof(*value->vector.values));
	for (size_t i = 0; i < value->vector.capacity; ++i) { value->vector.values[i] = 0; }
	return value;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone tables are openly addressed, linearly probed hash tables.      │
   │    Currently, lone tables use the FNV-1a hashing algorithm.            │
   │    They also strive to maintain a load factor of at most 0.5:          │
   │    tables will be rehashed once they're above half capacity.           │
   │    They do not use tombstones to delete keys.                          │
   │                                                                        │
   │    Tables are able to inherit from another table: missing keys         │
   │    are also looked up in the parent table. This is currently used      │
   │    to implement nested environments but will also serve as a           │
   │    prototype-based object system as in Javascript and Self.            │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_table_create(struct lone_lisp *lone, size_t capacity, struct lone_value *prototype)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_TABLE;
	value->table.prototype = prototype;
	value->table.capacity = capacity;
	value->table.count = 0;
	value->table.entries = lone_allocate(lone, capacity * sizeof(*value->table.entries));

	for (size_t i = 0; i < capacity; ++i) {
		value->table.entries[i].key = 0;
		value->table.entries[i].value = 0;
	}

	return value;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone integers are currently signed fixed-length integers.           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_integer_create(struct lone_lisp *lone, long integer)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_INTEGER;
	value->integer = integer;
	return value;
}

static struct lone_value *lone_integer_parse(struct lone_lisp *lone, unsigned char *digits, size_t count)
{
	size_t i = 0;
	long integer = 0;

	switch (*digits) { case '+': case '-': ++i; break; }

	while (i < count) {
		integer *= 10;
		integer += digits[i++] - '0';
	}

	if (*digits == '-') { integer *= -1; }

	return lone_integer_create(lone, integer);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone pointers do not own the data they point to.                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_pointer_create(struct lone_lisp *lone, void *pointer)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_POINTER;
	value->pointer = pointer;
	return value;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone texts are lone's strings and represent UTF-8 encoded text.     │
   │    Transfer and creation functions work like lone bytes.               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_text_transfer(struct lone_lisp *lone, unsigned char *text, size_t length)
{
	struct lone_value *value = lone_bytes_transfer(lone, text, length);
	value->type = LONE_TEXT;
	return value;
}

static struct lone_value *lone_text_transfer_bytes(struct lone_lisp *lone, struct lone_bytes bytes)
{
	return lone_text_transfer(lone, bytes.pointer, bytes.count);
}

static struct lone_value *lone_text_create(struct lone_lisp *lone, unsigned char *text, size_t length)
{
	struct lone_value *value = lone_bytes_create(lone, text, length);
	value->type = LONE_TEXT;
	return value;
}

static size_t lone_c_string_length(char *c_string)
{
	size_t length = 0;
	if (!c_string) { return 0; }
	while (c_string[length++]);
	return length - 1;
}

static struct lone_value *lone_text_create_from_c_string(struct lone_lisp *lone, char *c_string)
{
	return lone_text_create(lone, (unsigned char *) c_string, lone_c_string_length(c_string));
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone symbols are like lone texts but are interned in a table.       │
   │    Symbol table interning deduplicates them in memory,                 │
   │    enabling fast identity-based comparisons via pointer equality.      │
   │    However, this means they won't be garbage collected.                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_symbol_create(struct lone_lisp *lone, unsigned char *text, size_t length)
{
	struct lone_value *value = lone_bytes_create(lone, text, length);
	value->type = LONE_SYMBOL;
	return value;
}

static bool lone_is_nil(struct lone_value *);
static struct lone_value *lone_table_get(struct lone_lisp *, struct lone_value *, struct lone_value *);
static void lone_table_set(struct lone_lisp *, struct lone_value *, struct lone_value *, struct lone_value *);

static struct lone_value *lone_intern(struct lone_lisp *lone, unsigned char *bytes, size_t count)
{
	struct lone_value *key = lone_symbol_create(lone, bytes, count),
	                  *value = lone_table_get(lone, lone->symbol_table, key);

	if (lone_is_nil(value)) {
		value = key;
		lone_table_set(lone, lone->symbol_table, key, value);
	}

	return value;
}

static struct lone_value *lone_intern_c_string(struct lone_lisp *lone, char *c_string)
{
	return lone_intern(lone, (unsigned char *) c_string, lone_c_string_length(c_string));
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone modules are named isolated environments for evaluation.        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_module_create(struct lone_lisp *lone, struct lone_value *name)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_MODULE;
	value->module.name = name;
	value->module.environment = lone_table_create(lone, 64, 0);
	lone_table_set(lone, value->module.environment, lone_intern_c_string(lone, "import"), lone->modules.import);
	return value;
}

static struct lone_value *lone_primitive_import(struct lone_lisp *, struct lone_value *, struct lone_value *, struct lone_value *);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The lone lisp structure represents the lone lisp interpreter.       │
   │    A pointer to this structure is passed to nearly every function.     │
   │    It must be initialized before everything else since the memory      │
   │    allocation system is not functional without it.                     │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static void lone_lisp_initialize(struct lone_lisp *lone, unsigned char *memory, size_t size, size_t heap_size, void *stack)
{
	lone->memory.stack = stack;

	lone->memory.general = (struct lone_memory *) __builtin_assume_aligned(memory, LONE_ALIGNMENT);
	lone->memory.general->prev = lone->memory.general->next = 0;
	lone->memory.general->free = 1;
	lone->memory.general->size = size - sizeof(struct lone_memory);

	lone->memory.heaps = lone_allocate_heap(lone, heap_size);

	/* basic initialization done, can now use value creation functions */

	lone->symbol_table = lone_table_create(lone, 256, 0);
	lone->constants.nil = lone_list_create_nil(lone);
	lone->constants.truth = lone_intern_c_string(lone, "true");
	lone->modules.loaded = lone_table_create(lone, 32, 0);
	struct lone_function_flags import_flags = { .evaluate_arguments = 0, .evaluate_result = 0, .variable_arguments = 1 };
	lone->modules.import = lone_primitive_create(lone, "import", lone_primitive_import, 0, import_flags);
	lone->modules.path = lone_vector_create(lone, 8);
	lone->modules.null = lone_module_create(lone, 0);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Type predicate functions.                                           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static bool lone_has_same_type(struct lone_value *x, struct lone_value *y)
{
	return x->type == y->type;
}

static bool lone_is_function(struct lone_value *value)
{
	return value->type == LONE_FUNCTION;
}

static bool lone_is_primitive(struct lone_value *value)
{
	return value->type == LONE_PRIMITIVE;
}

static bool lone_is_applicable(struct lone_value *value)
{
	return lone_is_function(value) || lone_is_primitive(value);
}

static bool lone_is_list(struct lone_value *value)
{
	return value->type == LONE_LIST;
}

static bool lone_is_nil(struct lone_value *value)
{
	return lone_is_list(value) && value->list.first == 0 && value->list.rest == 0;
}

static bool lone_has_bytes(struct lone_value *value)
{
	return value->type == LONE_TEXT || value->type == LONE_SYMBOL || value->type == LONE_BYTES;
}

static bool lone_is_bytes(struct lone_value *value)
{
	return value->type == LONE_BYTES;
}

static bool lone_is_text(struct lone_value *value)
{
	return value->type == LONE_TEXT;
}

static bool lone_is_symbol(struct lone_value *value)
{
	return value->type == LONE_SYMBOL;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Comparison and equality functions.                                  │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static bool lone_is_identical(struct lone_value *x, struct lone_value *y)
{
	return x == y;
}

static bool lone_bytes_equals(struct lone_bytes x, struct lone_bytes y);

static bool lone_is_equivalent(struct lone_value *x, struct lone_value *y)
{
	if (lone_is_identical(x, y)) { return true; }
	if (!lone_has_same_type(x, y)) { return false; }

	switch (x->type) {
	case LONE_SYMBOL:
	case LONE_TEXT:
	case LONE_BYTES:
		return lone_bytes_equals(x->bytes, y->bytes);
	case LONE_INTEGER:
		return x->integer == y->integer;
	case LONE_POINTER:
		return x->pointer == y->pointer;

	case LONE_MODULE: case LONE_FUNCTION: case LONE_PRIMITIVE:
	case LONE_LIST: case LONE_VECTOR: case LONE_TABLE:
		return lone_is_identical(x, y);
	}
}

static bool lone_is_equal(struct lone_value *, struct lone_value *);

static bool lone_list_is_equal(struct lone_value *x, struct lone_value *y)
{
	return lone_is_equal(x->list.first, y->list.first) && lone_is_equal(x->list.rest, y->list.rest);
}

static bool lone_vector_is_equal(struct lone_value *x, struct lone_value *y)
{
	size_t i;

	if (x->vector.count != y->vector.count) return false;

	for (i = 0; i < x->vector.count; ++i) {
		if (!lone_is_equal(x->vector.values[i], y->vector.values[i])) {
			return false;
		}
	}

	return true;
}

static bool lone_table_is_equal(struct lone_value *x, struct lone_value *y)
{
	return lone_is_identical(x, y);
}

static bool lone_is_equal(struct lone_value *x, struct lone_value *y)
{
	if (lone_is_identical(x, y)) { return true; }
	if (!lone_has_same_type(x, y)) { return false; }

	switch (x->type) {
	case LONE_LIST:
		return lone_list_is_equal(x, y);
	case LONE_VECTOR:
		return lone_vector_is_equal(x, y);
	case LONE_TABLE:
		return lone_table_is_equal(x, y);

	case LONE_MODULE: case LONE_FUNCTION: case LONE_PRIMITIVE:
	case LONE_SYMBOL: case LONE_TEXT: case LONE_BYTES:
	case LONE_INTEGER: case LONE_POINTER:
		return lone_is_equivalent(x, y);
	}
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    List manipulation functions.                                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static inline struct lone_value *lone_list_first(struct lone_value *value)
{
	return value->list.first;
}

static inline struct lone_value *lone_list_rest(struct lone_value *value)
{
	return value->list.rest;
}

static struct lone_value *lone_list_set_first(struct lone_value *list, struct lone_value *value)
{
	return list->list.first = value;
}

static struct lone_value *lone_list_set_rest(struct lone_value *list, struct lone_value *rest)
{
	return list->list.rest = rest;
}

static struct lone_value *lone_list_build(struct lone_lisp *lone, size_t count, ...)
{
	struct lone_value *list = lone_list_create_nil(lone), *head = list, *argument;
	va_list arguments;
	size_t i;

	va_start(arguments, count);

	for (i = 0; i < count; ++i) {
		argument = va_arg(arguments, struct lone_value *);
		lone_list_set_first(head, argument);
		head = lone_list_set_rest(head, lone_list_create_nil(lone));
	}

	va_end(arguments);

	return list;
}

static struct lone_value *lone_list_flatten(struct lone_lisp *lone, struct lone_value *list)
{
	struct lone_value *flattened = lone_list_create_nil(lone), *head, *flat_head, *return_head, *first;

	for (head = list, flat_head = flattened; !lone_is_nil(head); head = lone_list_rest(head)) {
		first = lone_list_first(head);

		if (lone_is_list(first)) {
			return_head = lone_list_flatten(lone, first);

			for (/* return_head */; !lone_is_nil(return_head); return_head = lone_list_rest(return_head)) {
				lone_list_set_first(flat_head, lone_list_first(return_head));
				flat_head = lone_list_set_rest(flat_head, lone_list_create_nil(lone));
			}

		} else {
			lone_list_set_first(flat_head, first);
			flat_head = lone_list_set_rest(flat_head, lone_list_create_nil(lone));
		}
	}

	return flattened;
}

static bool lone_bytes_equals(struct lone_bytes x, struct lone_bytes y)
{
	if (x.count != y.count) return false;
	for (size_t i = 0; i < x.count; ++i) if (x.pointer[i] != y.pointer[i]) return false;
	return true;
}

static inline int lone_bytes_equals_c_string(struct lone_bytes bytes, char *c_string)
{
	struct lone_bytes c_string_bytes = { lone_c_string_length(c_string), (unsigned char *) c_string };
	return lone_bytes_equals(bytes, c_string_bytes);
}

static struct lone_bytes lone_join(struct lone_lisp *lone, struct lone_value *separator, struct lone_value *arguments, lone_predicate is_valid)
{
	struct lone_value *head, *argument;
	unsigned char *joined;
	size_t total = 0, position = 0;

	if (!is_valid) { is_valid = lone_has_bytes; }
	if (is_valid != lone_has_bytes && is_valid != lone_is_bytes &&
	    is_valid != lone_is_text   && is_valid != lone_is_symbol) {
		/* invalid predicate function */ linux_exit(-1);
	}

	if (separator && !lone_is_nil(separator)) {
		if (!is_valid(separator)) { linux_exit(-1); }
	}

	for (head = arguments; head && !lone_is_nil(head); head = lone_list_rest(head)) {
		argument = lone_list_first(head);
		if (!is_valid(argument)) { linux_exit(-1); }

		total += argument->bytes.count;
		if (separator && !lone_is_nil(separator)) {
			if (!lone_is_nil(lone_list_rest(head))) { total += separator->bytes.count; }
		}
	}

	joined = lone_allocate(lone, total + 1);

	for (head = arguments; head && !lone_is_nil(head); head = lone_list_rest(head)) {
		argument = lone_list_first(head);

		lone_memory_move(argument->bytes.pointer, joined + position, argument->bytes.count);
		position += argument->bytes.count;

		if (separator && !lone_is_nil(separator)) {
			if (!lone_is_nil(lone_list_rest(head))) {
				lone_memory_move(separator->bytes.pointer, joined + position, separator->bytes.count);
				position += separator->bytes.count;
			}
		}
	}

	joined[total] = '\0';

	return (struct lone_bytes) { .count = total, .pointer = joined };
}

static struct lone_bytes lone_concatenate(struct lone_lisp *lone, struct lone_value *arguments, lone_predicate is_valid)
{
	return lone_join(lone, 0, arguments, is_valid);
}

static void lone_vector_resize(struct lone_lisp *lone, struct lone_value *vector, size_t new_capacity)
{
	struct lone_value **new = lone_allocate(lone, new_capacity * sizeof(struct lone_value *));
	size_t i;

	for (i = 0; i < new_capacity; ++i) {
		new[i] = i < vector->vector.count? vector->vector.values[i] : 0;
	}

	lone_deallocate(lone, vector->vector.values);

	vector->vector.values = new;
	vector->vector.capacity = new_capacity;
	if (vector->vector.count > new_capacity) { vector->vector.count = new_capacity; }
}

static struct lone_value *lone_vector_get(struct lone_lisp *lone, struct lone_value *vector, struct lone_value *index)
{
	struct lone_value *value;
	size_t i;
	if (index->type != LONE_INTEGER) { /* only integer indexes supported */ linux_exit(-1); }
	i = index->integer;
	value = i < vector->vector.capacity? vector->vector.values[i] : 0;
	return value? value : lone_nil(lone);
}

static void lone_vector_set_value_at(struct lone_lisp *lone, struct lone_value *vector, size_t i, struct lone_value *value)
{
	if (i >= vector->vector.capacity) { lone_vector_resize(lone, vector, i * 2); }
	vector->vector.values[i] = value;
	if (++i > vector->vector.count) { vector->vector.count = i; }
}

static void lone_vector_set(struct lone_lisp *lone, struct lone_value *vector, struct lone_value *index, struct lone_value *value)
{
	if (index->type != LONE_INTEGER) { /* only integer indexes supported */ linux_exit(-1); }
	lone_vector_set_value_at(lone, vector, index->integer, value);
}

static void lone_vector_push(struct lone_lisp *lone, struct lone_value *vector, struct lone_value *value)
{
	lone_vector_set_value_at(lone, vector, vector->vector.count, value);
}

static unsigned long  __attribute__((pure)) fnv_1a(struct lone_bytes data)
{
	unsigned long hash = FNV_OFFSET_BASIS;
	unsigned char *bytes = data.pointer;
	size_t count = data.count;

	while (count--) {
		hash ^= *bytes++;
		hash *= FNV_PRIME;
	}

	return hash;
}

static inline size_t lone_table_hash(struct lone_value *key)
{
	struct lone_bytes bytes;
	unsigned long hash;

	if (!key) { /* a null key is probably a bug */ linux_exit(-1); }

	bytes.pointer = (unsigned char *) &key->type;
	bytes.count = sizeof(key->type);
	hash = fnv_1a(bytes);

	if (lone_is_nil(key)) { return hash; }

	switch (key->type) {
	case LONE_MODULE:
	case LONE_FUNCTION:
	case LONE_PRIMITIVE:
	case LONE_VECTOR:
	case LONE_TABLE:
		linux_exit(-1);
	case LONE_LIST:
		return hash ^ lone_table_hash(key->list.first) ^ lone_table_hash(key->list.rest);
	case LONE_SYMBOL:
	case LONE_TEXT:
	case LONE_BYTES:
		bytes = key->bytes;
		break;
	case LONE_INTEGER:
		bytes.pointer = (unsigned char *) &key->integer;
		bytes.count = sizeof(key->integer);
		break;
	case LONE_POINTER:
		bytes.pointer = (unsigned char *) &key->pointer;
		bytes.count = sizeof(key->pointer);
		break;
	}

	hash ^= fnv_1a(bytes);

	return hash;
}

static unsigned long lone_table_compute_hash_for(struct lone_value *key, size_t capacity)
{
	return lone_table_hash(key) % capacity;
}

static size_t lone_table_entry_find_index_for(struct lone_value *key, struct lone_table_entry *entries, size_t capacity)
{
	size_t i = lone_table_compute_hash_for(key, capacity);

	while (entries[i].key && !lone_is_equal(entries[i].key, key)) {
		i = (i + 1) % capacity;
	}

	return i;
}

static int lone_table_entry_set(struct lone_table_entry *entries, size_t capacity, struct lone_value *key, struct lone_value *value)
{
	size_t i = lone_table_entry_find_index_for(key, entries, capacity);
	struct lone_table_entry *entry = &entries[i];

	if (entry->key) {
		entry->value = value;
		return 0;
	} else {
		entry->key = key;
		entry->value = value;
		return 1;
	}
}

static void lone_table_resize(struct lone_lisp *lone, struct lone_value *table, size_t new_capacity)
{
	size_t old_capacity = table->table.capacity, i;
	struct lone_table_entry *old = table->table.entries,
	                        *new = lone_allocate(lone, new_capacity * sizeof(*new));

	for (i = 0; i < new_capacity; ++i) {
		new[i].key = 0;
		new[i].value = 0;
	}

	for (i = 0; i < old_capacity; ++i) {
		if (old[i].key) {
			lone_table_entry_set(new, new_capacity, old[i].key, old[i].value);
		}
	}

	lone_deallocate(lone, old);
	table->table.entries = new;
	table->table.capacity = new_capacity;
}

static void lone_table_set(struct lone_lisp *lone, struct lone_value *table, struct lone_value *key, struct lone_value *value)
{
	if (table->table.count >= table->table.capacity / 2) {
		lone_table_resize(lone, table, table->table.capacity * 2);
	}

	if (lone_table_entry_set(table->table.entries, table->table.capacity, key, value)) {
		++table->table.count;
	}
}

static struct lone_value *lone_table_get(struct lone_lisp *lone, struct lone_value *table, struct lone_value *key)
{
	size_t capacity = table->table.capacity, i;
	struct lone_table_entry *entries = table->table.entries, *entry;
	struct lone_value *prototype = table->table.prototype;

	i = lone_table_entry_find_index_for(key, entries, capacity);
	entry = &entries[i];

	if (entry->key) {
		return entry->value;
	} else if (prototype && !lone_is_nil(prototype)) {
		return lone_table_get(lone, prototype, key);
	} else {
		return lone_nil(lone);
	}
}

static void lone_table_delete(struct lone_lisp *lone, struct lone_value *table, struct lone_value *key)
{
	size_t capacity = table->table.capacity, i, j, k;
	struct lone_table_entry *entries = table->table.entries;

	i = lone_table_entry_find_index_for(key, entries, capacity);

	if (!entries[i].key) { return; }

	j = i;
	while (1) {
		j = (j + 1) % capacity;
		if (!entries[j].key) { break; }
		k = lone_table_compute_hash_for(entries[j].key, capacity);
		if ((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j))) {
			entries[i] = entries[j];
			i = j;
		}
	}

	entries[i].key = 0;
	entries[i].value = 0;
	--table->table.count;
}

/* ╭─────────────────────────┨ LONE LISP READER ┠───────────────────────────╮
   │                                                                        │
   │    The reader's job is to transform input into lone lisp values.       │
   │    It accomplishes the task by reading input from a given file         │
   │    descriptor and then lexing and parsing the results.                 │
   │                                                                        │
   │    The lexer or tokenizer transforms a linear stream of characters     │
   │    into a linear stream of tokens suitable for parser consumption.     │
   │    This gets rid of insignificant whitespace and reduces the size      │
   │    of the parser's input significantly.                                │
   │                                                                        │
   │    It consists of an input buffer, its current position in it          │
   │    as well as two functions:                                           │
   │                                                                        │
   │        ◦ peek(k) which returns the character at i+k                    │
   │        ◦ consume(k) which advances i by k positions                    │
   │                                                                        │
   │    The parser transforms a linear sequence of tokens into a nested     │
   │    sequence of lisp objects suitable for evaluation.                   │
   │    Its main task is to match nested structures such as lists.          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
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

static void lone_reader_initialize(struct lone_lisp *lone, struct lone_reader *reader, size_t buffer_size, int file_descriptor)
{
	reader->file_descriptor = file_descriptor;
	reader->buffer.bytes.count = buffer_size;
	reader->buffer.bytes.pointer = lone_allocate(lone, buffer_size);
	reader->buffer.position.read = 0;
	reader->buffer.position.write = 0;
	reader->error = 0;
}

static size_t lone_reader_fill_buffer(struct lone_lisp *lone, struct lone_reader *reader)
{
	unsigned char *buffer = reader->buffer.bytes.pointer;
	size_t size = reader->buffer.bytes.count, position = reader->buffer.position.write,
	       allocated = size, bytes_read = 0, total_read = 0;
	ssize_t read_result = 0;

	while (1) {
		read_result = linux_read(reader->file_descriptor, buffer + position, size);

		if (read_result < 0) {
			linux_exit(-1);
		}

		bytes_read = (size_t) read_result;
		total_read += bytes_read;
		position += bytes_read;

		if (bytes_read == size) {
			allocated += size;
			buffer = lone_reallocate(lone, buffer, allocated);
		} else {
			break;
		}
	}

	reader->buffer.bytes.pointer = buffer;
	reader->buffer.bytes.count = allocated;
	reader->buffer.position.write = position;
	return total_read;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The peek(k) function returns the k-th element from the input        │
   │    starting from the current input position, with peek(0) being        │
   │    the current character and peek(k) being look ahead for k > 1.       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static unsigned char *lone_reader_peek_k(struct lone_lisp *lone, struct lone_reader *reader, size_t k)
{
	size_t read_position = reader->buffer.position.read,
	       write_position = reader->buffer.position.write,
	       bytes_read;

	if (read_position + k >= write_position) {
		// we'd overrun the buffer because there's not enough input
		// fill it up by reading more first
		bytes_read = lone_reader_fill_buffer(lone, reader);
		if (bytes_read <= k) {
			// wanted at least k bytes but got less
			return 0;
		}
	}

	return reader->buffer.bytes.pointer + read_position + k;
}

static unsigned char *lone_reader_peek(struct lone_lisp *lone, struct lone_reader *reader)
{
	return lone_reader_peek_k(lone, reader, 0);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The consume(k) function advances the input position by k.           │
   │    This progresses through the input, consuming it.                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static void lone_reader_consume_k(struct lone_reader *reader, size_t k)
{
	reader->buffer.position.read += k;
}

static void lone_reader_consume(struct lone_reader *reader)
{
	lone_reader_consume_k(reader, 1);
}

static int lone_reader_match_byte(unsigned char byte, unsigned char target)
{
	if (target == ' ') {
		switch (byte) {
		case ' ':
		case '\t':
		case '\n':
			return 1;
		default:
			return 0;
		}
	} else if (target == ')' || target == ']' || target == '}') {
		return byte == ')' || byte == ']' || byte == '}';
	} else if (target >= '0' && target <= '9') {
		return byte >= '0' && byte <= '9';
	} else {
		return byte == target;
	}
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a number and adds it to the tokens list if valid.          │
   │                                                                        │
   │    ([+-]?[0-9]+)[)]} \n\t]                                             │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_reader_consume_number(struct lone_lisp *lone, struct lone_reader *reader)
{
	unsigned char *current, *start = lone_reader_peek(lone, reader);
	if (!start) { return 0; }
	size_t end = 0;

	switch (*start) {
	case '+': case '-':
		lone_reader_consume(reader);
		++end;
		break;
	default:
		break;
	}

	if ((current = lone_reader_peek(lone, reader)) && lone_reader_match_byte(*current, '1')) {
		lone_reader_consume(reader);
		++end;
	} else { return 0; }

	while ((current = lone_reader_peek(lone, reader)) && lone_reader_match_byte(*current, '1')) {
		lone_reader_consume(reader);
		++end;
	}

	if (current && !lone_reader_match_byte(*current, ')') && !lone_reader_match_byte(*current, ' ')) { return 0; }

	return lone_integer_parse(lone, start, end);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a symbol and adds it to the tokens list if valid.          │
   │                                                                        │
   │    (.*)[)]} \n\t]                                                      │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_reader_consume_symbol(struct lone_lisp *lone, struct lone_reader *reader)
{
	unsigned char *current, *start = lone_reader_peek(lone, reader);
	if (!start) { return 0; }
	size_t end = 0;

	while ((current = lone_reader_peek(lone, reader)) && !lone_reader_match_byte(*current, ')') && !lone_reader_match_byte(*current, ' ')) {
		lone_reader_consume(reader);
		++end;
	}

	return lone_intern(lone, start, end);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a string and adds it to the tokens list if valid.          │
   │                                                                        │
   │    (".*")[)]} \n\t]                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_reader_consume_text(struct lone_lisp *lone, struct lone_reader *reader)
{
	size_t end = 0;
	unsigned char *current, *start = lone_reader_peek(lone, reader);
	if (!start || *start != '"') { return 0; }

	// skip leading "
	++start;
	lone_reader_consume(reader);

	while ((current = lone_reader_peek(lone, reader)) && *current != '"') {
		lone_reader_consume(reader);
		++end;
	}

	// skip trailing "
	++current;
	lone_reader_consume(reader);

	if (!lone_reader_match_byte(*current, ')') && !lone_reader_match_byte(*current, ' ')) { return 0; }

	return lone_text_create(lone, start, end);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a single character token,                                  │
   │    characters that the parser deals with specially.                    │
   │    These include single quotes and opening and closing brackets.       │
   │                                                                        │
   │    (['()[]{}])                                                         │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_reader_consume_character(struct lone_lisp *lone, struct lone_reader *reader)
{
	unsigned char *bracket = lone_reader_peek(lone, reader);
	if (!bracket) { return 0; }

	switch (*bracket) {
	case '(': case ')':
	case '[': case ']':
	case '{': case '}':
	case '\'':
		lone_reader_consume(reader);
		return lone_intern(lone, bracket, 1);
	default:
		return 0;
	}
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The lone lisp lexer receives as input a single lone bytes value     │
   │    containing the full source code to be processed and it outputs      │
   │    a lone list of each lisp token found in the input. For example:     │
   │                                                                        │
   │        lex ← lone_bytes = [ (abc ("zxc") ]                             │
   │        lex → lone_list  = { ( → abc → ( → "zxc" → ) }                  │
   │                                                                        │
   │    Note that the list is linear and parentheses are not matched.       │
   │    The lexical analysis algorithm can be summarized as follows:        │
   │                                                                        │
   │        ◦ Skip all whitespace until it finds something                  │
   │        ◦ Fail if tokens aren't separated by spaces or ) at the end     │
   │        ◦ If found sign before digits tokenize signed number            │
   │        ◦ If found digit then look for more digits and tokenize         │
   │        ◦ If found " then find the next " and tokenize                  │
   │        ◦ If found ( or ) just tokenize them as is without matching     │
   │        ◦ Tokenize everything else unmodified as a symbol               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_lex(struct lone_lisp *lone, struct lone_reader *reader)
{
	struct lone_value *token = 0;
	unsigned char *c;

	while ((c = lone_reader_peek(lone, reader))) {
		if (lone_reader_match_byte(*c, ' ')) {
			lone_reader_consume(reader);
			continue;
		} else {
			unsigned char *c1;

			switch (*c) {
			case '+': case '-':
				if ((c1 = lone_reader_peek_k(lone, reader, 1)) && lone_reader_match_byte(*c1, '1')) {
					token = lone_reader_consume_number(lone, reader);
				} else {
					token = lone_reader_consume_symbol(lone, reader);
				}
				break;
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				token = lone_reader_consume_number(lone, reader);
				break;
			case '"':
				token = lone_reader_consume_text(lone, reader);
				break;
			case '(': case ')':
			case '[': case ']':
			case '{': case '}':
			case '\'':
				token = lone_reader_consume_character(lone, reader);
				break;
			default:
				token = lone_reader_consume_symbol(lone, reader);
				break;
			}

			if (token) {
				break;
			} else {
				goto lex_failed;
			}
		}
	}

	return token;

lex_failed:
	linux_exit(-1);
}

static struct lone_value *lone_parse(struct lone_lisp *, struct lone_reader *, struct lone_value *);

static struct lone_value *lone_parse_vector(struct lone_lisp *lone, struct lone_reader *reader)
{
	struct lone_value *vector = lone_vector_create(lone, 32), *value;
	size_t i = 0;

	while (1) {
		value = lone_lex(lone, reader);

		if (!value) { /* end of input */ reader->error = 1; return 0; }
		if (value->type == LONE_SYMBOL && *value->bytes.pointer == ']') {
			/* complete vector: [], [ x ], [ x y ] */
			break;
		}

		value = lone_parse(lone, reader, value);

		lone_vector_set_value_at(lone, vector, i++, value);
	}

	return vector;
}
static struct lone_value *lone_parse_table(struct lone_lisp *lone, struct lone_reader *reader)
{
	struct lone_value *table = lone_table_create(lone, 32, 0), *key, *value;

	while (1) {
		key = lone_lex(lone, reader);

		if (!key) { /* end of input */ reader->error = 1; return 0; }
		if (key->type == LONE_SYMBOL && *key->bytes.pointer == '}') {
			/* complete table: {}, { x y } */
			break;
		}

		key = lone_parse(lone, reader, key);

		value = lone_lex(lone, reader);

		if (!value) { /* end of input */ reader->error = 1; return 0; }
		if (value->type == LONE_SYMBOL && *value->bytes.pointer == '}') {
			/* incomplete table: { x }, { x y z } */
			reader->error = 1;
			return 0;
		}

		value = lone_parse(lone, reader, value);

		lone_table_set(lone, table, key, value);
	}

	return table;
}

static struct lone_value *lone_parse_list(struct lone_lisp *lone, struct lone_reader *reader)
{
	struct lone_value *list = lone_list_create_nil(lone), *first = list, *next;

	while (1) {
		next = lone_lex(lone, reader);
		if (!next) { reader->error = 1; return 0; }

		if (next->type == LONE_SYMBOL && *next->bytes.pointer == ')') {
			break;
		}

		lone_list_set_first(list, lone_parse(lone, reader, next));
		list = lone_list_set_rest(list, lone_list_create_nil(lone));
	}

	return first;
}

static struct lone_value *lone_parse_quote(struct lone_lisp *lone, struct lone_reader *reader)
{
	struct lone_value *quote = lone_intern_c_string(lone, "quote"),
	                  *value = lone_parse(lone, reader, lone_lex(lone, reader)),
	                  *form = lone_list_create(lone, value, lone_nil(lone));

	return lone_list_create(lone, quote, form);
}

static struct lone_value *lone_parse(struct lone_lisp *lone, struct lone_reader *reader, struct lone_value *token)
{
	if (!token) { return 0; }

	// lexer has already parsed atoms
	// parser deals with nested structures
	switch (token->type) {
	case LONE_SYMBOL:
		switch (*token->bytes.pointer) {
		case '(':
			return lone_parse_list(lone, reader);
		case '[':
			return lone_parse_vector(lone, reader);
		case '{':
			return lone_parse_table(lone, reader);
		case ')': case ']': case '}':
			goto parse_failed;
		case '\'':
			return lone_parse_quote(lone, reader);
		default:
			return token;
		}
	case LONE_INTEGER:
	case LONE_TEXT:
		return token;
	case LONE_MODULE:
	case LONE_FUNCTION:
	case LONE_PRIMITIVE:
	case LONE_LIST:
	case LONE_VECTOR:
	case LONE_TABLE:
	case LONE_BYTES:
	case LONE_POINTER:
		/* unexpected value type from lexer */
		goto parse_failed;
	}

parse_failed:
	linux_exit(-1);
}

static struct lone_value *lone_read(struct lone_lisp *lone, struct lone_reader *reader)
{
	return lone_parse(lone, reader, lone_lex(lone, reader));
}

/* ╭────────────────────────┨ LONE LISP EVALUATOR ┠─────────────────────────╮
   │                                                                        │
   │    The heart of the language. This is what actually executes code.     │
   │    Currently supports resolving variable references.                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_evaluate(struct lone_lisp *, struct lone_value *, struct lone_value *);

static struct lone_value *lone_evaluate_module(struct lone_lisp *lone, struct lone_value *module, struct lone_value *value)
{
	return lone_evaluate(lone, module->module.environment, value);
}

static struct lone_value *lone_evaluate_form_index(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *collection, struct lone_value *arguments)
{
	struct lone_value *(*get)(struct lone_lisp *, struct lone_value *, struct lone_value *);
	void (*set)(struct lone_lisp *, struct lone_value *, struct lone_value *, struct lone_value *);
	struct lone_value *key, *value;

	switch (collection->type) {
	case LONE_VECTOR:
		get = lone_vector_get;
		set = lone_vector_set;
		break;
	case LONE_TABLE:
		get = lone_table_get;
		set = lone_table_set;
		break;
	case LONE_MODULE: case LONE_FUNCTION: case LONE_PRIMITIVE:
	case LONE_BYTES: case LONE_SYMBOL: case LONE_TEXT:
	case LONE_LIST: case LONE_INTEGER: case LONE_POINTER:
		linux_exit(-1);
	}

	if (lone_is_nil(arguments)) { /* need at least the key: (collection) */ linux_exit(-1); }
	key = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(arguments)) {
		/* table get: (collection key) */
		return get(lone, collection, lone_evaluate(lone, environment, key));
	} else {
		/* at least one argument */
		value = lone_list_first(arguments);
		arguments = lone_list_rest(arguments);
		if (lone_is_nil(arguments)) {
			/* table set: (collection key value) */
			set(lone, collection,
			          lone_evaluate(lone, environment, key),
			          lone_evaluate(lone, environment, value));
			return value;
		} else {
			/* too many arguments given: (collection key value extra) */
			linux_exit(-1);
		}
	}
}

static struct lone_value *lone_apply(struct lone_lisp *, struct lone_value *, struct lone_value *, struct lone_value *);
static struct lone_value *lone_apply_function(struct lone_lisp *, struct lone_value *, struct lone_value *, struct lone_value *);
static struct lone_value *lone_apply_primitive(struct lone_lisp *, struct lone_value *, struct lone_value *, struct lone_value *);

static struct lone_value *lone_evaluate_form(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *list)
{
	struct lone_value *first = lone_list_first(list), *rest = lone_list_rest(list);

	/* apply arguments to a lone value */
	first = lone_evaluate(lone, environment, first);
	switch (first->type) {
	case LONE_FUNCTION:
	case LONE_PRIMITIVE:
		return lone_apply(lone, environment, first, rest);
	case LONE_VECTOR:
	case LONE_TABLE:
		return lone_evaluate_form_index(lone, environment, first, rest);
	case LONE_MODULE:
	case LONE_LIST:
	case LONE_SYMBOL:
	case LONE_TEXT:
	case LONE_BYTES:
	case LONE_INTEGER:
	case LONE_POINTER:
		/* first element not an applicable type */ linux_exit(-1);
	}
}

static struct lone_value *lone_evaluate(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *value)
{
	if (value == 0) { return 0; }
	if (lone_is_nil(value)) { return value; }

	switch (value->type) {
	case LONE_LIST:
		return lone_evaluate_form(lone, environment, value);
	case LONE_SYMBOL:
		return lone_table_get(lone, environment, value);
	case LONE_MODULE:
	case LONE_FUNCTION:
	case LONE_PRIMITIVE:
	case LONE_VECTOR:
	case LONE_TABLE:
	case LONE_INTEGER:
	case LONE_POINTER:
	case LONE_BYTES:
	case LONE_TEXT:
		return value;
	}
}

static struct lone_value *lone_evaluate_all(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *list)
{
	struct lone_value *evaluated = lone_list_create_nil(lone), *head;

	for (head = evaluated; !lone_is_nil(list); list = lone_list_rest(list)) {
		lone_list_set_first(head, lone_evaluate(lone, environment, lone_list_first(list)));
		head = lone_list_set_rest(head, lone_list_create_nil(lone));
	}

	return evaluated;
}

static struct lone_value *lone_apply(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *applicable, struct lone_value *arguments)
{
	if (!lone_is_applicable(applicable)) { /* given function is not an applicable type */ linux_exit(-1); }

	if (lone_is_function(applicable)) {
		return lone_apply_function(lone, environment, applicable, arguments);
	} else {
		return lone_apply_primitive(lone, environment, applicable, arguments);
	}
}

static struct lone_value *lone_apply_function(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *function, struct lone_value *arguments)
{
	struct lone_value *new_environment = lone_table_create(lone, 16, function->function.environment),
	                  *names = function->function.arguments, *code = function->function.code,
	                  *value = lone_nil(lone);

	if (function->function.flags.evaluate_arguments) { arguments = lone_evaluate_all(lone, environment, arguments); }

	if (function->function.flags.variable_arguments) {
		if (lone_is_nil(names) || !lone_is_nil(lone_list_rest(names))) {
			/* must have exactly one argument: the list of arguments */
			linux_exit(-1);
		}

		lone_table_set(lone, new_environment, lone_list_first(names), arguments);
	} else {
		while (1) {
			if (lone_is_nil(names) != lone_is_nil(arguments)) {
				/* argument number mismatch: ((lambda (x) x) 10 20), ((lambda (x y) y) 10) */
				linux_exit(-1);
			} else if (lone_is_nil(names) && lone_is_nil(arguments)) {
				break;
			}

			lone_table_set(lone, new_environment, lone_list_first(names), lone_list_first(arguments));

			names = lone_list_rest(names);
			arguments = lone_list_rest(arguments);
		}
	}

	while (1) {
		if (lone_is_nil(code)) { break; }
		value = lone_list_first(code);
		value = lone_evaluate(lone, new_environment, value);
		code = lone_list_rest(code);
	}

	if (function->function.flags.evaluate_result) { value = lone_evaluate(lone, environment, value); }

	return value;
}

static struct lone_value *lone_apply_primitive(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *primitive, struct lone_value *arguments)
{
	struct lone_value *result;
	if (primitive->primitive.flags.evaluate_arguments) { arguments = lone_evaluate_all(lone, environment, arguments); }
	result = primitive->primitive.function(lone, primitive->primitive.closure, environment, arguments);
	if (primitive->primitive.flags.evaluate_result) { result = lone_evaluate(lone, environment, result); }
	return result;
}

/* ╭─────────────────────────┨ LONE LISP PRINTER ┠──────────────────────────╮
   │                                                                        │
   │    Transforms lone lisp objects into text in order to write it out.    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static void lone_print(struct lone_lisp *, struct lone_value *, int);

static void lone_print_integer(int fd, long n)
{
	static char digits[DECIMAL_DIGITS_PER_LONG + 1]; /* digits, sign */
	char *digit = digits + DECIMAL_DIGITS_PER_LONG;  /* work backwards */
	size_t count = 0;
	int is_negative;

	if (n < 0) {
		is_negative = 1;
		n *= -1;
	} else {
		is_negative = 0;
	}

	do {
		*--digit = '0' + (n % 10);
		n /= 10;
		++count;
	} while (n > 0);

	if (is_negative) {
		*--digit = '-';
		++count;
	}

	linux_write(fd, digit, count);
}

static void lone_print_bytes(struct lone_lisp *lone, struct lone_value *bytes, int fd)
{
	size_t count = bytes->bytes.count;
	if (count == 0) { linux_write(fd, "bytes[]", 7); return; }

	static unsigned char hexadecimal[] = "0123456789ABCDEF";
	size_t size = 2 + count * 2; // size required: "0x" + 2 characters per input byte
	unsigned char *text = lone_allocate(lone, size);
	unsigned char *byte = bytes->bytes.pointer;
	size_t i;

	text[0] = '0';
	text[1] = 'x';

	for (i = 0; i < count; ++i) {
		unsigned char low  = (byte[i] & 0x0F) >> 0;
		unsigned char high = (byte[i] & 0xF0) >> 4;
		text[2 + (2 * i + 0)] = hexadecimal[high];
		text[2 + (2 * i + 1)] = hexadecimal[low];
	}

	linux_write(fd, "bytes[", 6);
	linux_write(fd, text, size);
	linux_write(fd, "]", 1);

	lone_deallocate(lone, text);
}

static void lone_print_list(struct lone_lisp *lone, struct lone_value *list, int fd)
{
	if (list == 0 || lone_is_nil(list)) { return; }

	struct lone_value *first = list->list.first,
	                  *rest  = list->list.rest;

	lone_print(lone, first, fd);

	if (rest->type == LONE_LIST) {
		if (!lone_is_nil(rest)) {
			linux_write(fd, " ", 1);
			lone_print_list(lone, rest, fd);
		}
	} else {
		linux_write(fd, " . ", 3);
		lone_print(lone, rest, fd);
	}
}

static void lone_print_vector(struct lone_lisp *lone, struct lone_value *vector, int fd)
{
	size_t n = vector->vector.count, i;
	struct lone_value **values = vector->vector.values;

	if (vector->vector.count == 0) { linux_write(fd, "[]", 2); return; }

	linux_write(fd, "[ ", 2);

	for (i = 0; i < n; ++i) {
		lone_print(lone, values[i], fd);
		linux_write(fd, " ", 1);
	}

	linux_write(fd, "]", 1);
}

static void lone_print_table(struct lone_lisp *lone, struct lone_value *table, int fd)
{
	size_t n = table->table.capacity, i;
	struct lone_table_entry *entries = table->table.entries;

	if (table->table.count == 0) { linux_write(fd, "{}", 2); return; }

	linux_write(fd, "{ ", 2);

	for (i = 0; i < n; ++i) {
		struct lone_value *key   = entries[i].key,
		                  *value = entries[i].value;


		if (key) {
			lone_print(lone, key, fd);
			linux_write(fd, " ", 1);
			lone_print(lone, value, fd);
			linux_write(fd, " ", 1);
		}
	}

	linux_write(fd, "}", 1);
}

static void lone_print_function(struct lone_lisp *lone, struct lone_value *function, int fd)
{
	struct lone_value *arguments = function->function.arguments,
	                  *code = function->function.code;

	linux_write(fd, "(𝛌 ", 6);
	lone_print(lone, arguments, fd);

	while (!lone_is_nil(code)) {
		linux_write(fd, "\n  ", 3);
		lone_print(lone, lone_list_first(code), fd);
		code = lone_list_rest(code);
	}

	linux_write(fd, ")", 1);
}

static void lone_print_hash_notation(struct lone_lisp *lone, char *descriptor, struct lone_value *value, int fd)
{
	linux_write(fd, "#<", 2);
	linux_write(fd, descriptor, lone_c_string_length(descriptor));
	linux_write(fd, " ", 1);
	lone_print(lone, value, fd);
	linux_write(fd, ">", 1);
}

static void lone_print(struct lone_lisp *lone, struct lone_value *value, int fd)
{
	if (value == 0) { return; }
	if (lone_is_nil(value)) { linux_write(fd, "nil", 3); return; }

	switch (value->type) {
	case LONE_MODULE:
		lone_print_hash_notation(lone, "module", value->module.name, fd);
		break;
	case LONE_PRIMITIVE:
		lone_print_hash_notation(lone, "primitive", value->primitive.name, fd);
		break;
	case LONE_FUNCTION:
		lone_print_function(lone, value, fd);
		break;
	case LONE_LIST:
		linux_write(fd, "(", 1);
		lone_print_list(lone, value, fd);
		linux_write(fd, ")", 1);
		break;
	case LONE_VECTOR:
		lone_print_vector(lone, value, fd);
		break;
	case LONE_TABLE:
		lone_print_table(lone, value, fd);
		break;
	case LONE_BYTES:
		lone_print_bytes(lone, value, fd);
		break;
	case LONE_SYMBOL:
		linux_write(fd, value->bytes.pointer, value->bytes.count);
		break;
	case LONE_TEXT:
		linux_write(fd, "\"", 1);
		linux_write(fd, value->bytes.pointer, value->bytes.count);
		linux_write(fd, "\"", 1);
		break;
	case LONE_INTEGER:
	case LONE_POINTER:
		lone_print_integer(fd, value->integer);
		break;
	}
}

/* ╭───────────────────┨ LONE LISP PRIMITIVE FUNCTIONS ┠────────────────────╮
   │                                                                        │
   │    Lone lisp functions implemented in C.                               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_primitive_begin(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *value;

	for (value = lone_nil(lone); !lone_is_nil(arguments); arguments = lone_list_rest(arguments)) {
		value = lone_list_first(arguments);
		value = lone_evaluate(lone, environment, value);
	}

	return value;
}

static struct lone_value *lone_primitive_when(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *test;

	if (lone_is_nil(arguments)) { /* test not specified: (when) */ linux_exit(-1); }
	test = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);

	if (!lone_is_nil(lone_evaluate(lone, environment, test))) {
		return lone_primitive_begin(lone, closure, environment, arguments);
	}

	return lone_nil(lone);
}

static struct lone_value *lone_primitive_unless(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *test;

	if (lone_is_nil(arguments)) { /* test not specified: (unless) */ linux_exit(-1); }
	test = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);

	if (lone_is_nil(lone_evaluate(lone, environment, test))) {
		return lone_primitive_begin(lone, closure, environment, arguments);
	}

	return lone_nil(lone);
}

static struct lone_value *lone_primitive_if(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *value, *consequent, *alternative = 0;

	if (lone_is_nil(arguments)) { /* test not specified: (if) */ linux_exit(-1); }
	value = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);

	if (lone_is_nil(arguments)) { /* consequent not specified: (if test) */ linux_exit(-1); }
	consequent = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);

	if (!lone_is_nil(arguments)) {
		alternative = lone_list_first(arguments);
		arguments = lone_list_rest(arguments);
		if (!lone_is_nil(arguments)) { /* too many values (if test consequent alternative extra) */ linux_exit(-1); }
	}

	if (!lone_is_nil(lone_evaluate(lone, environment, value))) {
		return lone_evaluate(lone, environment, consequent);
	} else if (alternative) {
		return lone_evaluate(lone, environment, alternative);
	}

	return lone_nil(lone);
}

static struct lone_value *lone_primitive_let(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *bindings, *first, *second, *rest, *value, *new_environment;

	if (lone_is_nil(arguments)) { /* no variables to bind: (let) */ linux_exit(-1); }
	bindings = lone_list_first(arguments);
	if (bindings->type != LONE_LIST) { /* expected list but got something else: (let 10) */ linux_exit(-1); }

	new_environment = lone_table_create(lone, 8, environment);

	while (1) {
		if (lone_is_nil(bindings)) { break; }
		first = lone_list_first(bindings);
		if (first->type != LONE_SYMBOL) { /* variable names must be symbols: (let ("x")) */ linux_exit(-1); }
		rest = lone_list_rest(bindings);
		if (lone_is_nil(rest)) { /* incomplete variable/value list: (let (x 10 y)) */ linux_exit(-1); }
		second = lone_list_first(rest);
		value = lone_evaluate(lone, new_environment, second);
		lone_table_set(lone, new_environment, first, value);
		bindings = lone_list_rest(rest);
	}

	value = lone_nil(lone);

	while (!lone_is_nil(arguments = lone_list_rest(arguments))) {
		value = lone_evaluate(lone, new_environment, lone_list_first(arguments));
	}

	return value;
}

static struct lone_value *lone_primitive_set(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *variable, *value;

	if (lone_is_nil(arguments)) {
		/* no variable to set: (set) */
		linux_exit(-1);
	}

	variable = lone_list_first(arguments);
	if (variable->type != LONE_SYMBOL) {
		/* variable names must be symbols: (set 10) */
		linux_exit(-1);
	}

	arguments = lone_list_rest(arguments);
	if (lone_is_nil(arguments)) {
		/* value not specified: (set variable) */
		value = lone_nil(lone);
	} else {
		/* (set variable value) */
		value = lone_list_first(arguments);
		arguments = lone_list_rest(arguments);
	}

	if (!lone_is_nil(arguments)) { /* too many arguments */ linux_exit(-1); }

	value = lone_evaluate(lone, environment, value);
	lone_table_set(lone, environment, variable, value);

	return value;
}

static struct lone_value *lone_primitive_quote(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	if (!lone_is_nil(lone_list_rest(arguments))) { /* too many arguments: (quote x y) */ linux_exit(-1); }
	return lone_list_first(arguments);
}

static struct lone_value *lone_primitive_lambda_with_flags(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments, struct lone_function_flags flags)
{
	struct lone_value *bindings, *code;

	bindings = lone_list_first(arguments);
	if (bindings->type != LONE_LIST) { /* parameters not a list: (lambda 10) */ linux_exit(-1); }

	code = lone_list_rest(arguments);

	return lone_function_create(lone, bindings, code, environment, flags);
}

static struct lone_value *lone_primitive_lambda(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_function_flags flags = {
		.evaluate_arguments = 1,
		.evaluate_result = 0,
		.variable_arguments = 0,
	};

	return lone_primitive_lambda_with_flags(lone, closure, environment, arguments, flags);
}

static struct lone_value *lone_primitive_lambda_bang(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_function_flags flags = {
		.evaluate_arguments = 0,
		.evaluate_result = 0,
		.variable_arguments = 0,
	};

	return lone_primitive_lambda_with_flags(lone, closure, environment, arguments, flags);
}

static struct lone_value *lone_primitive_lambda_star(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_function_flags flags = {
		.evaluate_arguments = 1,
		.evaluate_result = 0,
		.variable_arguments = 1,
	};

	return lone_primitive_lambda_with_flags(lone, closure, environment, arguments, flags);
}

static struct lone_value *lone_apply_equality_function(struct lone_lisp *lone, struct lone_value *arguments, lone_comparator equal)
{
	struct lone_value *argument, *next;

	while (1) {
		if (lone_is_nil(arguments)) { break; }
		argument = lone_list_first(arguments);
		arguments = lone_list_rest(arguments);
		next = lone_list_first(arguments);

		if (next && !equal(argument, next)) { return lone_nil(lone); }
	}

	return lone_true(lone);
}

static struct lone_value *lone_primitive_is_identical(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	return lone_apply_equality_function(lone, arguments, lone_is_identical);
}

static struct lone_value *lone_primitive_is_equivalent(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	return lone_apply_equality_function(lone, arguments, lone_is_equivalent);
}

static struct lone_value *lone_primitive_is_equal(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	return lone_apply_equality_function(lone, arguments, lone_is_equal);
}

static struct lone_value *lone_primitive_print(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	while (!lone_is_nil(arguments)) {
		lone_print(lone, lone_list_first(arguments), 1);
		linux_write(1, "\n", 1);
		arguments = lone_list_rest(arguments);
	}

	return lone_nil(lone);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Built-in mathematical and numeric operations.                       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_primitive_integer_operation(struct lone_lisp *lone, struct lone_value *arguments, char operation)
{
	struct lone_value *argument;
	long accumulator;

	switch (operation) {
	case '+': case '-': accumulator = 0; break;
	case '*': accumulator = 1; break;
	default: /* invalid primitive integer operation */ linux_exit(-1);
	}

	if (lone_is_nil(arguments)) { /* wasn't given any arguments to operate on: (+), (-), (*) */ goto return_accumulator; }

	do {
		argument = lone_list_first(arguments);
		if (argument->type != LONE_INTEGER) { /* argument is not a number */ linux_exit(-1); }

		switch (operation) {
		case '+': accumulator += argument->integer; break;
		case '-': accumulator -= argument->integer; break;
		case '*': accumulator *= argument->integer; break;
		default: /* invalid primitive integer operation */ linux_exit(-1);
		}

		arguments = lone_list_rest(arguments);
	} while (!lone_is_nil(arguments));

return_accumulator:
	return lone_integer_create(lone, accumulator);
}

static struct lone_value *lone_primitive_add(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	return lone_primitive_integer_operation(lone, arguments, '+');
}

static struct lone_value *lone_primitive_subtract(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	return lone_primitive_integer_operation(lone, arguments, '-');
}

static struct lone_value *lone_primitive_multiply(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	return lone_primitive_integer_operation(lone, arguments, '*');
}

static struct lone_value *lone_primitive_divide(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *dividend, *divisor;

	if (lone_is_nil(arguments)) { /* at least the dividend is required, (/) is invalid */ linux_exit(-1); }
	dividend = lone_list_first(arguments);
	if (dividend->type != LONE_INTEGER) { /* can't divide non-numbers: (/ "not a number") */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);

	if (lone_is_nil(arguments)) {
		/* not given a divisor, return 1/x instead: (/ 2) = 1/2 */
		return lone_integer_create(lone, 1 / dividend->integer);
	} else {
		/* (/ x a b c ...) = x / (a * b * c * ...) */
		divisor = lone_primitive_integer_operation(lone, arguments, '*');
		return lone_integer_create(lone, dividend->integer / divisor->integer);
	}
}

static struct lone_value *lone_primitive_sign(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *value;
	if (lone_is_nil(arguments)) { /* no arguments: (sign) */ linux_exit(-1); }
	value = lone_list_first(arguments);
	if (!lone_is_nil(lone_list_rest(arguments))) { /* too many arguments: (sign 1 2 3) */ linux_exit(-1); }

	if (value->type == LONE_INTEGER) {
		return lone_integer_create(lone, value->integer > 0? 1 : value->integer < 0? -1 : 0);
	} else {
		linux_exit(-1);
	}
}

static struct lone_value *lone_primitive_is_zero(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *value = lone_primitive_sign(lone, closure, environment, arguments);
	if (value->type == LONE_INTEGER && value->integer == 0) { return value; }
	else { return lone_nil(lone); }
}

static struct lone_value *lone_primitive_is_positive(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *value = lone_primitive_sign(lone, closure, environment, arguments);
	if (value->type == LONE_INTEGER && value->integer > 0) { return value; }
	else { return lone_nil(lone); }
}

static struct lone_value *lone_primitive_is_negative(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *value = lone_primitive_sign(lone, closure, environment, arguments);
	if (value->type == LONE_INTEGER && value->integer < 0) { return value; }
	else { return lone_nil(lone); }
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Text operations.                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_primitive_join(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	return lone_text_transfer_bytes(lone, lone_join(lone, lone_list_first(arguments), lone_list_rest(arguments), lone_is_text));
}

static struct lone_value *lone_primitive_concatenate(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	return lone_text_transfer_bytes(lone, lone_concatenate(lone, arguments, lone_is_text));
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    List operations.                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_primitive_first(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *argument;
	if (lone_is_nil(arguments)) { linux_exit(-1); }
	argument = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(argument)) { linux_exit(-1); }
	if (!lone_is_nil(arguments)) { linux_exit(-1); }
	return lone_list_first(argument);
}

static struct lone_value *lone_primitive_rest(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *argument;
	if (lone_is_nil(arguments)) { linux_exit(-1); }
	argument = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(argument)) { linux_exit(-1); }
	if (!lone_is_nil(arguments)) { linux_exit(-1); }
	return lone_list_rest(argument);
}

static struct lone_value *lone_primitive_flatten(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	return lone_list_flatten(lone, arguments);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Module importing and loading operations.                            │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static void lone_primitive_import_all(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *module)
{
	/* full import, bind all symbols: (import (module)) */
	struct lone_table_entry *entries = module->module.environment->table.entries;
	size_t i, capacity = module->module.environment->table.capacity;

	for (i = 0; i < capacity; ++i) {
		if (entries[i].key) {
			lone_table_set(lone, environment, entries[i].key, entries[i].value);
		}
	}
}

static void lone_primitive_import_only(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *module, struct lone_value *symbols)
{
	struct lone_value *symbol, *value;

	/* limited import, bind only specified symbols: (import (module x f)) */
	do {
		symbol = lone_list_first(symbols);
		if (symbol->type != LONE_SYMBOL) { /* name not a symbol: (import (module 10)) */ linux_exit(-1); }

		value = lone_table_get(lone, module->module.environment, symbol);
		if (lone_is_nil(value)) { /* name not set in module */ linux_exit(-1); }

		lone_table_set(lone, environment, symbol, value);

	} while (!lone_is_nil(symbols = lone_list_rest(symbols)));
}

static struct lone_value *lone_module_load(struct lone_lisp *lone, struct lone_value *name);

static void lone_primitive_import_argument(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *argument)
{
	struct lone_value *name, *module;

	if (lone_is_nil(argument)) { /* nothing to import: (import ()) */ linux_exit(-1); }

	switch (argument->type) {
	case LONE_SYMBOL:
		name = argument;
		argument = 0;
		break;
	case LONE_LIST:
		name = lone_list_first(argument);
		argument = lone_list_rest(argument);
		break;
	case LONE_MODULE:
	case LONE_FUNCTION: case LONE_PRIMITIVE:
	case LONE_TEXT: case LONE_BYTES:
	case LONE_VECTOR: case LONE_TABLE:
	case LONE_INTEGER: case LONE_POINTER:
		/* not a supported import argument type */ linux_exit(-1);
	}

	module = lone_module_load(lone, name);
	if (lone_is_nil(module)) { /* module not found: (import non-existent), (import (non-existent)) */ linux_exit(-1); }

	if (argument) {
		/* (import (module)), (import (module symbol)) */
		if (lone_is_nil(argument)) {
			/* (import (module)) */
			lone_primitive_import_all(lone, environment, module);
		} else {
			/* (import (module symbol)) */
			lone_primitive_import_only(lone, environment, module, argument);
		}
	} else {
		/* (import module) */
		lone_primitive_import_all(lone, environment, module);
	}
}

static struct lone_value *lone_primitive_import(struct lone_lisp *lone, struct lone_value *closure, struct lone_value *environment, struct lone_value *arguments)
{
	if (lone_is_nil(arguments)) { /* nothing to import: (import) */ linux_exit(-1); }

	for (/* argument */; !lone_is_nil(arguments); arguments = lone_list_rest(arguments)) {
		lone_primitive_import_argument(lone, environment, lone_list_first(arguments));
	}

	return lone_nil(lone);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Linux primitive functions for issuing system calls.                 │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static inline long lone_value_to_linux_system_call_number(struct lone_lisp *lone, struct lone_value *linux_system_call_table, struct lone_value *value)
{
	switch (value->type) {
	case LONE_INTEGER:
		return value->integer;
	case LONE_BYTES:
	case LONE_TEXT:
	case LONE_SYMBOL:
		return lone_table_get(lone, linux_system_call_table, value)->integer;
	case LONE_MODULE:
	case LONE_FUNCTION:
	case LONE_PRIMITIVE:
	case LONE_LIST:
	case LONE_VECTOR:
	case LONE_TABLE:
	case LONE_POINTER:
		linux_exit(-1);
	}
}

static inline long lone_value_to_linux_system_call_argument(struct lone_value *value)
{
	switch (value->type) {
	case LONE_INTEGER: return value->integer;
	case LONE_POINTER: return (long) value->pointer;
	case LONE_BYTES: case LONE_TEXT: case LONE_SYMBOL: return (long) value->bytes.pointer;
	case LONE_PRIMITIVE: return (long) value->primitive.function;
	case LONE_FUNCTION: case LONE_LIST: case LONE_VECTOR: case LONE_TABLE: case LONE_MODULE: linux_exit(-1);
	}
}

static struct lone_value *lone_primitive_linux_system_call(struct lone_lisp *lone, struct lone_value *linux_system_call_table, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *argument;
	long result, number, args[6];
	unsigned char i;

	if (lone_is_nil(arguments)) { /* need at least the system call number */ linux_exit(-1); }
	argument = lone_list_first(arguments);
	number = lone_value_to_linux_system_call_number(lone, linux_system_call_table, argument);
	arguments = lone_list_rest(arguments);

	for (i = 0; i < 6; ++i) {
		if (lone_is_nil(arguments)) {
			args[i] = 0;
		} else {
			argument = lone_list_first(arguments);
			args[i] = lone_value_to_linux_system_call_argument(argument);
			arguments = lone_list_rest(arguments);
		}
	}

	if (!lone_is_nil(arguments)) { /* too many arguments given */ linux_exit(-1); }

	result = system_call_6(number, args[0], args[1], args[2], args[3], args[4], args[5]);

	return lone_integer_create(lone, result);
}

/* ╭─────────────────────────┨ LONE LINUX PROCESS ┠─────────────────────────╮
   │                                                                        │
   │    Code to access all the parameters Linux passes to its processes.    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct auxiliary {
	long type;
	union {
		char *c_string;
		void *pointer;
		long integer;
	} as;
};

static void lone_auxiliary_value_to_table(struct lone_lisp *lone, struct lone_value *table, struct auxiliary *auxiliary_value)
{
	struct lone_value *key, *value;
	switch (auxiliary_value->type) {
	case AT_BASE_PLATFORM:
		key = lone_intern_c_string(lone, "base-platform");
		value = lone_text_create_from_c_string(lone, auxiliary_value->as.c_string);
		break;
	case AT_PLATFORM:
		key = lone_intern_c_string(lone, "platform");
		value = lone_text_create_from_c_string(lone, auxiliary_value->as.c_string);
		break;
	case AT_HWCAP:
		key = lone_intern_c_string(lone, "hardware-capabilities");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_HWCAP2:
		key = lone_intern_c_string(lone, "hardware-capabilities-2");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_FLAGS:
		key = lone_intern_c_string(lone, "flags");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_NOTELF:
		key = lone_intern_c_string(lone, "not-ELF");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_BASE:
		key = lone_intern_c_string(lone, "interpreter-base-address");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer);
		break;
	case AT_ENTRY:
		key = lone_intern_c_string(lone, "entry-point");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer);
		break;
	case AT_SYSINFO_EHDR:
		key = lone_intern_c_string(lone, "vDSO");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer);
		break;
	case AT_PHDR:
		key = lone_intern_c_string(lone, "program-headers-address");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer);
		break;
	case AT_PHENT:
		key = lone_intern_c_string(lone, "program-headers-entry-size");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_PHNUM:
		key = lone_intern_c_string(lone, "program-headers-count");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_EXECFN:
		key = lone_intern_c_string(lone, "executable-file-name");
		value = lone_text_create_from_c_string(lone, auxiliary_value->as.c_string);
		break;
	case AT_EXECFD:
		key = lone_intern_c_string(lone, "executable-file-descriptor");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_UID:
		key = lone_intern_c_string(lone, "user-id");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_EUID:
		key = lone_intern_c_string(lone, "effective-user-id");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_GID:
		key = lone_intern_c_string(lone, "group-id");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_EGID:
		key = lone_intern_c_string(lone, "effective-group-id");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_PAGESZ:
		key = lone_intern_c_string(lone, "page-size");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
#ifdef AT_MINSIGSTKSZ
	case AT_MINSIGSTKSZ:
		key = lone_intern_c_string(lone, "minimum-signal-delivery-stack-size");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
#endif
	case AT_CLKTCK:
		key = lone_intern_c_string(lone, "clock-tick");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_RANDOM:
		key = lone_intern_c_string(lone, "random");
		value = lone_bytes_create(lone, auxiliary_value->as.pointer, 16);
		break;
	case AT_SECURE:
		key = lone_intern_c_string(lone, "secure");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	default:
		key = lone_intern_c_string(lone, "unknown");
		value = lone_list_create(lone,
		                         lone_integer_create(lone, auxiliary_value->type),
		                         lone_integer_create(lone, auxiliary_value->as.integer));
	}

	lone_table_set(lone, table, key, value);
}

static struct lone_value *lone_auxiliary_vector_to_table(struct lone_lisp *lone, struct auxiliary *auxiliary_values)
{
	struct lone_value *table = lone_table_create(lone, 32, 0);
	size_t i;

	for (i = 0; auxiliary_values[i].type != AT_NULL; ++i) {
		lone_auxiliary_value_to_table(lone, table, &auxiliary_values[i]);
	}

	return table;
}

static struct lone_value *lone_environment_to_table(struct lone_lisp *lone, char **c_strings)
{
	struct lone_value *table = lone_table_create(lone, 64, 0), *key, *value;
	char *c_string_key, *c_string_value, *c_string;

	for (/* c_strings */; *c_strings; ++c_strings) {
		c_string = *c_strings;
		c_string_key = c_string;
		c_string_value = "";

		while (*c_string++) {
			if (*c_string == '=') {
				*c_string = '\0';
				c_string_value = c_string + 1;
				break;
			}
		}

		key = lone_text_create_from_c_string(lone, c_string_key);
		value = lone_text_create_from_c_string(lone, c_string_value);
		lone_table_set(lone, table, key, value);
	}

	return table;
}

static struct lone_value *lone_arguments_to_list(struct lone_lisp *lone, int count, char **c_strings)
{
	struct lone_value *arguments = lone_list_create_nil(lone), *head;
	int i;

	for (i = 0, head = arguments; i < count; ++i) {
		lone_list_set_first(head, lone_text_create_from_c_string(lone, c_strings[i]));
		head = lone_list_set_rest(head, lone_list_create_nil(lone));
	}

	return arguments;
}

static void lone_fill_linux_system_call_table(struct lone_lisp *lone, struct lone_value *linux_system_call_table)
{
	size_t i;

	static struct linux_system_call {
		char *symbol;
		int number;
	} linux_system_calls[] = {

		/* huge generated array initializer with all the system calls found on the host platform */
		#include LONE_NR_SOURCE

	};

	for (i = 0; i < (sizeof(linux_system_calls)/sizeof(linux_system_calls[0])); ++i) {
		lone_table_set(lone, linux_system_call_table,
		               lone_intern_c_string(lone, linux_system_calls[i].symbol),
		               lone_integer_create(lone, linux_system_calls[i].number));
	}
}

/* ╭─────────────────────────┨ LONE LISP MODULES ┠──────────────────────────╮
   │                                                                        │
   │    Built-in modules containing essential functionality.                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_module_null(struct lone_lisp *lone)
{
	return lone->modules.null;
}

static struct lone_value *lone_module_name_to_key(struct lone_lisp *lone, struct lone_value *name)
{
	struct lone_value *head;

	switch (name->type) {
	case LONE_SYMBOL:
		return lone_list_create(lone, name, lone_nil(lone));
	case LONE_LIST:
		for (head = name; !lone_is_nil(head); head = lone_list_rest(head)) {
			if (!lone_is_symbol(lone_list_first(head))) {
				linux_exit(-1);
			}
		}
		return name;
	case LONE_MODULE:
		return lone_module_name_to_key(lone, name->module.name);
	case LONE_TEXT: case LONE_BYTES:
	case LONE_FUNCTION: case LONE_PRIMITIVE:
	case LONE_VECTOR: case LONE_TABLE:
	case LONE_INTEGER: case LONE_POINTER:
		linux_exit(-1);
	}
}

static struct lone_value *lone_module_for_name(struct lone_lisp *lone, struct lone_value *name, bool *not_found)
{
	struct lone_value *module;

	name = lone_module_name_to_key(lone, name);
	module = lone_table_get(lone, lone->modules.loaded, name);
	*not_found = false;

	if (lone_is_nil(module)) {
		module = lone_module_create(lone, name);
		lone_table_set(lone, lone->modules.loaded, name, module);
		*not_found = true;
	}

	return module;
}

static int lone_module_search(struct lone_lisp *lone, struct lone_value *symbols)
{
	struct lone_value *slash = lone_intern_c_string(lone, "/"), *ln = lone_intern_c_string(lone, ".ln");
	struct lone_value *arguments, *package, *search_path;
	unsigned char *path;
	size_t i;
	long result;

	symbols = lone_module_name_to_key(lone, symbols);
	package = lone_list_first(symbols);

	for (i = 0; i < lone->modules.path->vector.count; ++i) {
		search_path = lone->modules.path->vector.values[i];
		arguments = lone_list_build(lone, 3, search_path, package, symbols);
		arguments = lone_list_flatten(lone, arguments);
		arguments = lone_text_transfer_bytes(lone, lone_join(lone, slash, arguments, lone_has_bytes));
		arguments = lone_list_build(lone, 2, arguments, ln);
		path = lone_concatenate(lone, arguments, lone_has_bytes).pointer;

		result = linux_openat(AT_FDCWD, path, O_RDONLY | O_CLOEXEC);

		lone_deallocate(lone, path);

		switch (result) {
		case -ENOENT:
		case -EACCES: case -EPERM:
		case -ENOTDIR: case -EISDIR:
		case -EINVAL: case -ENAMETOOLONG:
		case -EMFILE: case -ENFILE:
		case -ELOOP:
			continue;
		case -ENOMEM: case -EFAULT:
			linux_exit(-1);
		}

		return (int) result;
	}

	linux_exit(-1); /* module not found */
}

static void lone_module_load_from_file_descriptor(struct lone_lisp *lone, struct lone_value *module, int file_descriptor)
{
	struct lone_value *value;
	struct lone_reader reader;

	lone_reader_initialize(lone, &reader, LONE_BUFFER_SIZE, file_descriptor);

	while (1) {
		value = lone_read(lone, &reader);
		if (!value) { if (reader.error) { linux_exit(-1); } else { break; } }

		value = lone_evaluate_module(lone, module, value);
	}

	lone_garbage_collector(lone);
}

static struct lone_value *lone_module_load(struct lone_lisp *lone, struct lone_value *name)
{
	struct lone_value *module;
	bool not_found;
	int file_descriptor;

	module = lone_module_for_name(lone, name, &not_found);

	if (not_found) {
		file_descriptor = lone_module_search(lone, name);
		lone_module_load_from_file_descriptor(lone, module, file_descriptor);
		linux_close(file_descriptor);
	}

	return module;
}

static void lone_builtin_module_linux_initialize(struct lone_lisp *lone, int argc, char **argv, char **envp, struct auxiliary *auxv)
{
	struct lone_value *name = lone_module_name_to_key(lone, lone_intern_c_string(lone, "linux")),
	                  *module = lone_module_create(lone, name),
	                  *linux_system_call_table = lone_table_create(lone, 1024, 0),
	                  *count, *arguments, *environment, *auxiliary_values;

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "system-call"),
	                     lone_primitive_create(lone,
	                                           "linux_system_call",
	                                           lone_primitive_linux_system_call,
	                                           linux_system_call_table,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "system-call-table"),
	                     linux_system_call_table);

	lone_fill_linux_system_call_table(lone, linux_system_call_table);

	count = lone_integer_create(lone, argc);
	arguments = lone_arguments_to_list(lone, argc, argv);
	environment = lone_environment_to_table(lone, envp);
	auxiliary_values = lone_auxiliary_vector_to_table(lone, auxv);

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "argument-count"),
	                     count);
	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "arguments"),
	                     arguments);
	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "environment"),
	                     environment);
	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "auxiliary-values"),
	                     auxiliary_values);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_builtin_module_math_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_module_name_to_key(lone, lone_intern_c_string(lone, "math")),
	                  *module = lone_module_create(lone, name);

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "+"),
	                     lone_primitive_create(lone,
	                                           "add",
	                                           lone_primitive_add,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "-"),
	                     lone_primitive_create(lone,
	                                           "subtract",
	                                           lone_primitive_subtract,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "*"),
	                     lone_primitive_create(lone,
	                                           "multiply",
	                                           lone_primitive_multiply,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "/"),
	                     lone_primitive_create(lone,
	                                           "divide",
	                                           lone_primitive_divide,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "sign"),
	                     lone_primitive_create(lone,
	                                           "sign",
	                                           lone_primitive_sign,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "zero?"),
	                     lone_primitive_create(lone,
	                                           "is_zero",
	                                           lone_primitive_is_zero,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "positive?"),
	                     lone_primitive_create(lone,
	                                           "is_positive",
	                                           lone_primitive_is_positive,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "negative?"),
	                     lone_primitive_create(lone,
	                                           "is_negative",
	                                           lone_primitive_is_negative,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_builtin_module_text_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_module_name_to_key(lone, lone_intern_c_string(lone, "text")),
	                  *module = lone_module_create(lone, name);

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "join"),
	                     lone_primitive_create(lone,
	                                           "join",
	                                           lone_primitive_join,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "concatenate"),
	                     lone_primitive_create(lone,
	                                           "concatenate",
	                                           lone_primitive_concatenate,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_builtin_module_list_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_module_name_to_key(lone, lone_intern_c_string(lone, "list")),
	                  *module = lone_module_create(lone, name);

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "first"),
	                     lone_primitive_create(lone,
	                                           "first",
	                                           lone_primitive_first,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "rest"),
	                     lone_primitive_create(lone,
	                                           "rest",
	                                           lone_primitive_rest,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "flatten"),
	                     lone_primitive_create(lone,
	                                           "flatten",
	                                           lone_primitive_flatten,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_builtin_module_lone_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_module_name_to_key(lone, lone_intern_c_string(lone, "lone")),
	                  *module = lone_module_create(lone, name);

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "begin"),
	                     lone_primitive_create(lone,
	                                           "begin",
	                                           lone_primitive_begin,
	                                           module,
	                                           (struct lone_function_flags) { 0, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "when"),
	                     lone_primitive_create(lone,
	                                           "when",
	                                           lone_primitive_when,
	                                           module,
	                                           (struct lone_function_flags) { 0, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "unless"),
	                     lone_primitive_create(lone,
	                                           "unless",
	                                           lone_primitive_unless,
	                                           module,
	                                           (struct lone_function_flags) { 0, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "if"),
	                     lone_primitive_create(lone,
	                                           "if",
	                                           lone_primitive_if,
	                                           module,
	                                           (struct lone_function_flags) { 0, 0, 0 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "let"),
	                     lone_primitive_create(lone,
	                                           "let",
	                                           lone_primitive_let,
	                                           module,
	                                           (struct lone_function_flags) { 0, 0, 0 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "set"),
	                     lone_primitive_create(lone,
	                                           "set",
	                                           lone_primitive_set,
	                                           module,
	                                           (struct lone_function_flags) { 0, 0, 0 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "quote"),
	                     lone_primitive_create(lone,
	                                           "quote",
	                                           lone_primitive_quote,
	                                           module,
	                                           (struct lone_function_flags) { 0, 0, 0 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "lambda"),
	                     lone_primitive_create(lone,
	                                           "lambda",
	                                           lone_primitive_lambda,
	                                           module,
	                                           (struct lone_function_flags) { 0, 0, 0 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "lambda!"),
	                     lone_primitive_create(lone,
	                                           "lambda_bang",
	                                           lone_primitive_lambda_bang,
	                                           module,
	                                           (struct lone_function_flags) { 0, 0, 0 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "lambda*"),
	                     lone_primitive_create(lone,
	                                           "lambda_star",
	                                           lone_primitive_lambda_star,
	                                           module,
	                                           (struct lone_function_flags) { 0, 0, 0 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "print"),
	                     lone_primitive_create(lone,
	                                           "print",
	                                           lone_primitive_print,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "identical?"),
	                     lone_primitive_create(lone,
	                                           "is_identical",
	                                           lone_primitive_is_identical,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "equivalent?"),
	                     lone_primitive_create(lone,
	                                           "is_equivalent",
	                                           lone_primitive_is_equivalent,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, module->module.environment,
	                     lone_intern_c_string(lone, "equal?"),
	                     lone_primitive_create(lone,
	                                           "is_equal",
	                                           lone_primitive_is_equal,
	                                           module,
	                                           (struct lone_function_flags) { 1, 0, 1 }));

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_modules_initialize(struct lone_lisp *lone, int argc, char **argv, char **envp, struct auxiliary *auxv)
{
	lone_builtin_module_linux_initialize(lone, argc, argv, envp, auxv);
	lone_builtin_module_lone_initialize(lone);
	lone_builtin_module_math_initialize(lone);
	lone_builtin_module_text_initialize(lone);
	lone_builtin_module_list_initialize(lone);

	lone_vector_push(lone, lone->modules.path, lone_text_create_from_c_string(lone, "."));
}

/* ╭───────────────────────┨ LONE LISP ENTRY POINT ┠────────────────────────╮
   │                                                                        │
   │    Linux places argument, environment and auxiliary value arrays       │
   │    on the stack before jumping to the entry point of the process.      │
   │    Architecture-specific code collects this data and passes it to      │
   │    the lone function which begins execution of the lisp code.          │
   │                                                                        │
   │    During early initialization, lone has no dynamic memory             │
   │    allocation capabilities and so this function statically             │
   │    allocates 64 KiB of memory for the early bootstrapping process.     │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
long lone(int argc, char **argv, char **envp, struct auxiliary *auxv)
{
	void *stack = __builtin_frame_address(0);
	static unsigned char __attribute__((aligned(LONE_ALIGNMENT))) memory[LONE_MEMORY_SIZE];
	struct lone_lisp lone;

	lone_lisp_initialize(&lone, memory, sizeof(memory), 1024, stack);
	lone_modules_initialize(&lone, argc, argv, envp, auxv);

	lone_module_load_from_file_descriptor(&lone, lone_module_null(&lone), 0);

	return 0;
}
