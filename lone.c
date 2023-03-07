/* SPDX-License-Identifier: AGPL-3.0-or-later */

/* ╭─────────────────────────────┨ LONE LISP ┠──────────────────────────────╮
   │                                                                        │
   │                       The standalone Linux Lisp                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#include <linux/types.h>
#include <linux/unistd.h>
#include <linux/auxvec.h>

typedef __kernel_size_t size_t;
typedef __kernel_ssize_t ssize_t;

static void __attribute__((noreturn)) linux_exit(int code)
{
	system_call_1(__NR_exit, code);
	__builtin_unreachable();
}

static ssize_t __attribute__((fd_arg_read(1))) __attribute__((tainted_args)) linux_read(int fd, const void *buffer, size_t count)
{
	return system_call_3(__NR_read, fd, (long) buffer, (long) count);
}

static ssize_t __attribute__((fd_arg_write(1))) __attribute__((tainted_args)) linux_write(int fd, const void *buffer, size_t count)
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

/* ╭──────────────────────────┨ LONE LISP TYPES ┠───────────────────────────╮
   │                                                                        │
   │    Lone implements dynamic data types as a tagged union.               │
   │    Supported types are:                                                │
   │                                                                        │
   │        ◦ List       the linked list and tree type                      │
   │        ◦ Table      the hash table, prototype and object type          │
   │        ◦ Symbol     the keyword and interned string type               │
   │        ◦ Text       the UTF-8 encoded text type                        │
   │        ◦ Bytes      the binary data and low level string type          │
   │        ◦ Integer    the signed integer type                            │
   │        ◦ Pointer    the memory addressing and dereferencing type       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
enum lone_type {
	LONE_MODULE,
	LONE_FUNCTION,
	LONE_PRIMITIVE,
	LONE_LIST,
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

struct lone_function {
	struct lone_value *arguments;      /* the bindings */
	struct lone_value *code;           /* the lambda */
	struct lone_value *environment;    /* the closure */
};

struct lone_lisp;
typedef struct lone_value *(*lone_primitive)(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *arguments);

struct lone_module {
	struct lone_value *name;
	struct lone_value *environment;
};

struct lone_value {
	enum lone_type type;
	union {
		struct lone_module module;
		struct lone_function function;
		lone_primitive primitive;
		struct lone_list list;
		struct lone_table table;
		struct lone_bytes bytes;   /* also used by texts and symbols */
		long integer;
		void *pointer;
	};
};

/* ╭───────────────────────┨ LONE LISP INTERPRETER ┠────────────────────────╮
   │                                                                        │
   │    The lone lisp interpreter is composed of all internal state         │
   │    necessary to process useful programs. It includes memory,           │
   │    the top level lisp environment and references to all objects.       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_memory;
struct lone_lisp {
	struct lone_memory *memory;
	struct lone_value_container *values;
	struct lone_value *environment;
	struct lone_value *modules;
	struct lone_value *symbol_table;
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
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_memory {
	struct lone_memory *prev, *next;
	int free;
	size_t size;
	unsigned char pointer[];
};

struct lone_value_header {
	struct lone_value_container *next;
	unsigned char marked;
};

struct lone_value_container {
	struct lone_value_header header;
	struct lone_value value;
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
		struct lone_memory *new = (struct lone_memory *) (block->pointer + used);
		new->next = block->next;
		new->prev = block;
		new->free = 1;
		new->size = excess - sizeof(struct lone_memory);
		block->next = new;
		block->size -= excess;
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

static void * __attribute__((malloc)) __attribute__((alloc_size(2))) lone_allocate(struct lone_lisp *lone, size_t requested_size)
{
	size_t needed_size = requested_size + sizeof(struct lone_memory);
	struct lone_memory *block;

	for (block = lone->memory; block; block = block->next) {
		if (block->free && block->size >= needed_size)
			break;
	}

	if (!block) { linux_exit(-1); }

	block->free = 0;
	lone_memory_split(block, needed_size);

	return block->pointer;
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

static inline struct lone_value_container *lone_value_to_container(struct lone_value* value)
{
	if (!value) { return 0; }
	unsigned char *bytes = (unsigned char *) value;
	bytes -= __builtin_offsetof(struct lone_value_container, value);
	return (struct lone_value_container *) bytes;
}

static void lone_mark_value(struct lone_value *value)
{
	struct lone_value_container *container = lone_value_to_container(value);

	if (!container || container->header.marked == 1) { return; }

	container->header.marked = 1;

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
	case LONE_LIST:
		lone_mark_value(container->value.list.first);
		lone_mark_value(container->value.list.rest);
		break;
	case LONE_TABLE:
		lone_mark_value(container->value.table.prototype);
		for (size_t i = 0; i < container->value.table.capacity; ++i) {
			lone_mark_value(container->value.table.entries[i].key);
			lone_mark_value(container->value.table.entries[i].value);
		}
		break;
	default:
		break;
	}
}

static void lone_mark_all_reachable_values(struct lone_lisp *lone)
{
	lone_mark_value(lone->environment);
	lone_mark_value(lone->modules);
	lone_mark_value(lone->symbol_table);
}

static void lone_deallocate_all_unmarked_values(struct lone_lisp *lone)
{
	struct lone_value_container **values = &lone->values, *value;
	while ((value = *values)) {
		if (value->header.marked) {
			value->header.marked = 0;
			values = &value->header.next;
		} else {
			*values = value->header.next;

			switch (value->value.type) {
			case LONE_BYTES:
			case LONE_TEXT:
			case LONE_SYMBOL:
				lone_deallocate(lone, value->value.bytes.pointer);
				break;
			case LONE_TABLE:
				lone_deallocate(lone, value->value.table.entries);
				break;
			}

			lone_deallocate(lone, value);
		}
	}
}

static void lone_garbage_collector(struct lone_lisp *lone)
{
	lone_mark_all_reachable_values(lone);
	lone_deallocate_all_unmarked_values(lone);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Initializers and creation functions for lone's types.               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static void lone_lisp_initialize(struct lone_lisp *lone, unsigned char *memory, size_t size)
{
	lone->memory = (struct lone_memory *) memory;
	lone->memory->prev = lone->memory->next = 0;
	lone->memory->free = 1;
	lone->memory->size = size - sizeof(struct lone_memory);
	lone->values = 0;
	lone->environment = 0;
	lone->modules = 0;
	lone->symbol_table = 0;
}

static struct lone_value *lone_value_create(struct lone_lisp *lone)
{
	struct lone_value_container *container = lone_allocate(lone, sizeof(struct lone_value_container));
	if (lone->values) { container->header.next = lone->values; }
	lone->values = container;
	container->header.marked = 0;
	return &container->value;
}

static struct lone_value *lone_bytes_create(struct lone_lisp *lone, unsigned char *pointer, size_t count)
{
	struct lone_value *value = lone_value_create(lone);
	unsigned char *copy = lone_allocate(lone, count);
	lone_memory_move(pointer, copy, count);
	value->type = LONE_BYTES;
	value->bytes.count = count;
	value->bytes.pointer = copy;
	return value;
}

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

static struct lone_value *lone_function_create(struct lone_lisp *lone, struct lone_value *arguments, struct lone_value *code, struct lone_value *environment)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_FUNCTION;
	value->function.arguments = arguments;
	value->function.code = code;
	value->function.environment = environment;
	return value;
}

static struct lone_value *lone_primitive_create(struct lone_lisp *lone, lone_primitive primitive)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_PRIMITIVE;
	value->primitive = primitive;
	return value;
}

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

static struct lone_value *lone_pointer_create(struct lone_lisp *lone, void *pointer)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_POINTER;
	value->pointer = pointer;
	return value;
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

static struct lone_value *lone_symbol_create(struct lone_lisp *lone, unsigned char *text, size_t length)
{
	struct lone_value *value = lone_bytes_create(lone, text, length);
	value->type = LONE_SYMBOL;
	return value;
}

static struct lone_value *lone_symbol_create_from_c_string(struct lone_lisp *lone, char *c_string)
{
	return lone_symbol_create(lone, (unsigned char*) c_string, lone_c_string_length(c_string));
}

static struct lone_value *lone_module_create(struct lone_lisp *lone, struct lone_value *name)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_MODULE;
	value->module.name = name;
	value->module.environment = lone_table_create(lone, 64, 0);
	return value;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Functions for operating on lone's built-in types.                   │
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

static int lone_is_nil(struct lone_value *value)
{
	return value->type == LONE_LIST && value->list.first == 0 && value->list.rest == 0;
}

static struct lone_value *lone_list_set(struct lone_value *list, struct lone_value *value)
{
	return list->list.first = value;
}

static struct lone_value *lone_list_append(struct lone_value *list, struct lone_value *rest)
{
	return list->list.rest = rest;
}

static int lone_bytes_equals(struct lone_bytes x, struct lone_bytes y)
{
	if (x.count != y.count) return 0;
	for (size_t i = 0; i < x.count; ++i) if (x.pointer[i] != y.pointer[i]) return 0;
	return 1;
}

static inline int lone_bytes_equals_c_string(struct lone_bytes bytes, char *c_string)
{
	struct lone_bytes c_string_bytes = { lone_c_string_length(c_string), (unsigned char *) c_string };
	return lone_bytes_equals(bytes, c_string_bytes);
}

static unsigned long  __attribute__((pure)) fnv_1a(unsigned char *bytes, size_t count)
{
	unsigned long hash = FNV_OFFSET_BASIS;

	while (count--) {
		hash ^= *bytes++;
		hash *= FNV_PRIME;
	}

	return hash;
}

static inline size_t lone_table_compute_hash_for(struct lone_value *key, size_t capacity)
{
	return fnv_1a(key->bytes.pointer, key->bytes.count) % capacity;
}

static size_t lone_table_entry_find_index_for(struct lone_value *key, struct lone_table_entry *entries, size_t capacity)
{
	size_t i = lone_table_compute_hash_for(key, capacity);

	while (entries[i].key && !lone_bytes_equals(entries[i].key->bytes, key->bytes)) {
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
		return lone_list_create_nil(lone);
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

static struct lone_value *lone_intern(struct lone_lisp *lone, unsigned char *bytes, size_t count)
{
	struct lone_value *key = lone_bytes_create(lone, bytes, count),
	                  *value = lone_table_get(lone, lone->symbol_table, key);

	if (lone_is_nil(value)) {
		value = lone_symbol_create(lone, bytes, count);
		lone_table_set(lone, lone->symbol_table, key, value);
	}

	return value;
}

static struct lone_value *lone_intern_c_string(struct lone_lisp *lone, char *c_string)
{
	return lone_intern(lone, (unsigned char *) c_string, lone_c_string_length(c_string));
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
   │    ([+-]?[0-9]+)[) \n\t]                                               │
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

	if (current && *current != ')' && !lone_reader_match_byte(*current, ' ')) { return 0; }

	return lone_integer_parse(lone, start, end);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a symbol and adds it to the tokens list if valid.          │
   │                                                                        │
   │    (.*)[) \n\t]                                                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_reader_consume_symbol(struct lone_lisp *lone, struct lone_reader *reader)
{
	unsigned char *current, *start = lone_reader_peek(lone, reader);
	if (!start) { return 0; }
	size_t end = 0;

	while ((current = lone_reader_peek(lone, reader)) && *current != ')' && !lone_reader_match_byte(*current, ' ')) {
		lone_reader_consume(reader);
		++end;
	}

	return lone_intern(lone, start, end);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a string and adds it to the tokens list if valid.          │
   │                                                                        │
   │    (".*")[) \n\t]                                                      │
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

	if (*current != ')' && !lone_reader_match_byte(*current, ' ')) { return 0; }

	return lone_text_create(lone, start, end);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes opening and closing parentheses                            │
   │    and adds them to the tokens list if valid.                          │
   │                                                                        │
   │    ([()])                                                              │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_reader_consume_parenthesis(struct lone_lisp *lone, struct lone_reader *reader)
{
	unsigned char *parenthesis = lone_reader_peek(lone, reader);
	if (!parenthesis) { return 0; }

	switch (*parenthesis) {
	case '(': case ')':
		lone_reader_consume(reader);
		return lone_intern(lone, parenthesis, 1);
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
			case '(':
			case ')':
				token = lone_reader_consume_parenthesis(lone, reader);
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

static struct lone_value *lone_parse_list(struct lone_lisp *lone, struct lone_reader *reader)
{
	struct lone_value *list = lone_list_create_nil(lone), *first = list, *next;

	while (1) {
		next = lone_lex(lone, reader);
		if (!next) { reader->error = 1; return 0; }

		if (next->type == LONE_SYMBOL && *next->bytes.pointer == ')') {
			break;
		}

		lone_list_set(list, lone_parse(lone, reader, next));
		list = lone_list_append(list, lone_list_create_nil(lone));
	}

	return first;
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
		case ')':
			goto parse_failed;
		default:
			return token;
		}
	case LONE_INTEGER:
	case LONE_TEXT:
		return token;
	default:
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

static struct lone_value *lone_evaluate_special_form_lambda(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *list)
{
	struct lone_value *arguments;

	arguments = lone_list_first(list);
	if (arguments->type != LONE_LIST) { /* parameters not a list: (lambda 10) */ linux_exit(-1); }

	list = lone_list_rest(list);
	if (lone_is_nil(list)) { /* no code: (lambda (x)) */ linux_exit(-1); }

	return lone_function_create(lone, arguments, list, environment);
}

static struct lone_value *lone_evaluate_special_form_if(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *list)
{
	struct lone_value *value, *consequent, *alternative = 0;

	if (lone_is_nil(list)) { /* empty if form: (if) */ linux_exit(-1); }
	value = lone_list_first(list);
	list = lone_list_rest(list);

	if (lone_is_nil(list)) { /* consequent not specified: (if test) */ linux_exit(-1); }
	consequent = lone_list_first(list);
	list = lone_list_rest(list);

	if (!lone_is_nil(list)) {
		alternative = lone_list_first(list);
		list = lone_list_rest(list);
		if (!lone_is_nil(list)) { /* too many values (if test consequent alternative extra) */ linux_exit(-1); }
	}

	if (!lone_is_nil(lone_evaluate(lone, environment, value))) {
		return lone_evaluate(lone, environment, consequent);
	} else if (alternative) {
		return lone_evaluate(lone, environment, alternative);
	}

	return lone_list_create_nil(lone);
}

static struct lone_value *lone_evaluate_special_form_let(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *list)
{
	struct lone_value *bindings, *first, *second, *rest, *value, *new_environment;

	if (lone_is_nil(list)) { /* empty let form: (let) */ linux_exit(-1); }
	bindings = lone_list_first(list);
	if (bindings->type != LONE_LIST) { /* expected list but got something else like: (let 10) */ linux_exit(-1); }

	new_environment = lone_table_create(lone, 8, environment);

	while (1) {
		if (lone_is_nil(bindings)) { break; }
		first = lone_list_first(bindings);
		if (first->type != LONE_SYMBOL) { /* variable names must be symbols */ linux_exit(-1); }
		rest = lone_list_rest(bindings);
		if (lone_is_nil(rest)) { /* incomplete variable/value list: (let (x 10 y)) */ linux_exit(-1); }
		second = lone_list_first(rest);
		value = lone_evaluate(lone, new_environment, second);
		lone_table_set(lone, new_environment, first, value);
		bindings = lone_list_rest(rest);
	}

	value = lone_list_create_nil(lone);

	while (!lone_is_nil(list = lone_list_rest(list))) {
		value = lone_evaluate(lone, new_environment, lone_list_first(list));
	}

	return value;
}

static struct lone_value *lone_evaluate_special_form_set(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *list)
{
	struct lone_value *variable, *value;

	if (lone_is_nil(list)) {
		// empty set form: (set)
		linux_exit(-1);
	}

	variable = lone_list_first(list);
	if (variable->type != LONE_SYMBOL) {
		// variable names must be symbols
		linux_exit(-1);
	}

	list = lone_list_rest(list);
	if (lone_is_nil(list)) {
		// value not specified: (set variable)
		linux_exit(-1);
	}

	// (set variable value)
	value = lone_evaluate(lone, environment, lone_list_first(list));
	lone_table_set(lone, environment, variable, value);

	return value;
}

static struct lone_value *lone_apply(struct lone_lisp *, struct lone_value *, struct lone_value *, struct lone_value *);

static struct lone_value *lone_evaluate_form(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *list)
{
	struct lone_value *first = lone_list_first(list), *rest = lone_list_rest(list);

	// check for special forms
	if (first->type == LONE_SYMBOL) {
		if (lone_bytes_equals_c_string(first->bytes, "set")) {
			return lone_evaluate_special_form_set(lone, environment, rest);
		} else if (lone_bytes_equals_c_string(first->bytes, "let")) {
			return lone_evaluate_special_form_let(lone, environment, rest);
		} else if (lone_bytes_equals_c_string(first->bytes, "if")) {
			return lone_evaluate_special_form_if(lone, environment, rest);
		} else if (lone_bytes_equals_c_string(first->bytes, "lambda")) {
			return lone_evaluate_special_form_lambda(lone, environment, rest);
		}
	}

	/* must be a function application if not a special form */
	first = lone_evaluate(lone, environment, first);
	switch (first->type) {
	case LONE_FUNCTION:
		return lone_apply(lone, environment, first, rest);
	case LONE_PRIMITIVE:
		return first->primitive(lone, environment, rest);
	default:
		/* first element must be a function */ linux_exit(-1);
	}
}

static struct lone_value *lone_evaluate(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *value)
{
	if (value == 0) { return 0; }

	switch (value->type) {
	case LONE_LIST:
		return lone_evaluate_form(lone, environment, value);
	case LONE_MODULE:
	case LONE_FUNCTION:
	case LONE_PRIMITIVE:
	case LONE_TABLE:
	case LONE_INTEGER:
	case LONE_POINTER:
	case LONE_BYTES:
	case LONE_TEXT:
		return value;
	case LONE_SYMBOL:
		return lone_table_get(lone, environment, value);
	}
}

static struct lone_value *lone_apply(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *function, struct lone_value *arguments)
{
	struct lone_value *new_environment = lone_table_create(lone, 16, function->function.environment),
	                  *names = function->function.arguments, *code = function->function.code,
	                  *value;

	while (1) {
		if (lone_is_nil(names) != lone_is_nil(arguments)) {
			/* argument number mismatch: ((lambda (x) x) 10 20), ((lambda (x y) y) 10) */
			linux_exit(-1);
		} else if (lone_is_nil(names) && lone_is_nil(arguments)) {
			break;
		}

		value = lone_evaluate(lone, environment, lone_list_first(arguments));
		lone_table_set(lone, new_environment, lone_list_first(names), value);

		names = lone_list_rest(names);
		arguments = lone_list_rest(arguments);
	}

	do {
		value = lone_evaluate(lone, new_environment, lone_list_first(code));
	} while (!lone_is_nil(code = lone_list_rest(code)));

	return value;
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

static void lone_print_table(struct lone_lisp *lone, struct lone_value *table, int fd)
{
	size_t n = table->table.capacity, i;
	struct lone_table_entry *entries = table->table.entries;

	linux_write(fd, "{ ", 2);

	for (i = 0; i < n; ++i) {
		struct lone_value *key   = entries[i].key,
		                  *value = entries[i].value;


		if (key) {
			lone_print(lone, entries[i].key, fd);
			linux_write(fd, " ", 1);
			if (value) { lone_print(lone, entries[i].value, fd); }
			else { linux_write(fd, "nil", 3); }
			linux_write(fd, " ", 1);
		}
	}

	linux_write(fd, "}", 1);
}

static void lone_print(struct lone_lisp *lone, struct lone_value *value, int fd)
{
	if (value == 0) { return; }
	if (lone_is_nil(value)) { linux_write(fd, "nil", 3); return; }

	switch (value->type) {
	case LONE_LIST:
		linux_write(fd, "(", 1);
		lone_print_list(lone, value, fd);
		linux_write(fd, ")", 1);
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
static inline long lone_value_to_linux_system_call_argument(struct lone_value *value)
{
	switch (value->type) {
	case LONE_INTEGER: return value->integer;
	case LONE_POINTER: return (long) value->pointer;
	case LONE_BYTES: case LONE_TEXT: case LONE_SYMBOL: return (long) value->bytes.pointer;
	case LONE_PRIMITIVE: return (long) value->primitive;
	case LONE_FUNCTION: case LONE_LIST: case LONE_TABLE: case LONE_MODULE: linux_exit(-1);
	}
}

static struct lone_value *lone_primitive_linux_system_call(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *argument;
	long result, number, args[6];
	unsigned char i;

	if (lone_is_nil(arguments)) { /* need at least the system call number */ linux_exit(-1); }
	argument = lone_list_first(arguments);
	if (argument->type != LONE_INTEGER) { /* system call number is not a number */ linux_exit(-1); }
	number = argument->integer;
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

static struct lone_value *lone_primitive_print(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *arguments)
{
	while (!lone_is_nil(arguments)) {
		lone_print(lone, lone_evaluate(lone, environment, lone_list_first(arguments)), 1);
		linux_write(1, "\n", 1);
		arguments = lone_list_rest(arguments);
	}

	return lone_list_create_nil(lone);
}

static struct lone_value *lone_primitive_integer_operation(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *arguments, char operation)
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

static struct lone_value *lone_primitive_add(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *arguments)
{
	return lone_primitive_integer_operation(lone, environment, arguments, '+');
}

static struct lone_value *lone_primitive_subtract(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *arguments)
{
	return lone_primitive_integer_operation(lone, environment, arguments, '-');
}

static struct lone_value *lone_primitive_multiply(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *arguments)
{
	return lone_primitive_integer_operation(lone, environment, arguments, '*');
}

static struct lone_value *lone_primitive_divide(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *arguments)
{
	struct lone_value *dividend, *divisor;

	if (lone_is_nil(arguments)) { /* at least the dividend is required, (/) is invalid */ linux_exit(-1); }
	dividend = lone_list_first(arguments);
	if (dividend->type != LONE_INTEGER) { /* can't divide non-numbers: (/ "not a number) */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);

	if (lone_is_nil(arguments)) {
		/* not given a divisor, return 1/x instead: (/ 2) = 1/2 */
		return lone_integer_create(lone, 1 / dividend->integer);
	} else {
		/* (/ x a b c ...) = x / (a * b * c * ...) */
		divisor = lone_primitive_integer_operation(lone, environment, arguments, '*');
		return lone_integer_create(lone, dividend->integer / divisor->integer);
	}
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
		lone_list_set(head, lone_text_create_from_c_string(lone, c_strings[i]));
		head = lone_list_append(head, lone_list_create_nil(lone));
	}

	return arguments;
}

static void lone_set_environment(struct lone_lisp *lone, struct lone_value *arguments, struct lone_value *environment, struct lone_value *auxiliary_values)
{
	struct lone_value *table = lone_table_create(lone, 16, 0);

	lone_table_set(lone, table, lone_intern_c_string(lone, "system-call"), lone_primitive_create(lone, lone_primitive_linux_system_call));
	lone_table_set(lone, table, lone_intern_c_string(lone, "print"), lone_primitive_create(lone, lone_primitive_print));

	lone_table_set(lone, table, lone_intern_c_string(lone, "+"), lone_primitive_create(lone, lone_primitive_add));
	lone_table_set(lone, table, lone_intern_c_string(lone, "-"), lone_primitive_create(lone, lone_primitive_subtract));
	lone_table_set(lone, table, lone_intern_c_string(lone, "*"), lone_primitive_create(lone, lone_primitive_multiply));
	lone_table_set(lone, table, lone_intern_c_string(lone, "/"), lone_primitive_create(lone, lone_primitive_divide));

	lone_table_set(lone, table, lone_intern_c_string(lone, "arguments"), arguments);
	lone_table_set(lone, table, lone_intern_c_string(lone, "environment"), environment);
	lone_table_set(lone, table, lone_intern_c_string(lone, "auxiliary-values"), auxiliary_values);

	lone->environment = table;
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
	#define LONE_BUFFER_SIZE 4096
	#define LONE_MEMORY_SIZE 65536
	static unsigned char memory[LONE_MEMORY_SIZE];
	struct lone_lisp lone;
	struct lone_reader reader;

	lone_lisp_initialize(&lone, memory, sizeof(memory));
	lone.modules = lone_table_create(&lone, 32, 0);
	lone.symbol_table = lone_table_create(&lone, 256, 0);

	struct lone_value *arguments = lone_arguments_to_list(&lone, argc, argv);
	struct lone_value *environment = lone_environment_to_table(&lone, envp);
	struct lone_value *auxiliary_values = lone_auxiliary_vector_to_table(&lone, auxv);

	lone_set_environment(&lone, arguments, environment, auxiliary_values);

	lone_reader_initialize(&lone, &reader, LONE_BUFFER_SIZE, 0);

	while (1) {
		struct lone_value *value = lone_read(&lone, &reader);
		if (!value) {
			if (reader.error) {
				return -1;
			} else {
				break;
			}
		}

		value = lone_evaluate(&lone, lone.environment, value);

		lone_garbage_collector(&lone);
	}

	return 0;
}
