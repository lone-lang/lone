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

static ssize_t linux_read(int fd, const void *buffer, size_t count)
{
	return system_call_3(__NR_read, fd, (long) buffer, (long) count);
}

static ssize_t linux_write(int fd, const void *buffer, size_t count)
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
   │        ◦ Symbol     the keyword and interned string type               │
   │        ◦ Text       the UTF-8 encoded text type                        │
   │        ◦ Bytes      the binary data and low level string type          │
   │        ◦ Integer    the signed integer type                            │
   │        ◦ Pointer    the memory addressing and dereferencing type       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
enum lone_type {
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
};

struct lone_value {
	enum lone_type type;
	union {
		struct lone_list list;
		struct lone_table table;
		struct lone_bytes bytes;   /* also used by texts and symbols */
		long integer;
		void *pointer;
	};
};

/* ╭────────────────────┨ LONE LISP MEMORY ALLOCATION ┠─────────────────────╮
   │                                                                        │
   │    Lone is designed to work without any dependencies except Linux,     │
   │    so it does not make use of even the system's C library.             │
   │    In order to bootstrap itself in such harsh conditions,              │
   │    it must be given some memory to work with.                          │
   │    The lone_linux structure holds that memory.                         │
   │                                                                        │
   │    Lone will allocate from its internal memory at first.               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_memory {
	struct lone_memory *prev, *next;
	int free;
	size_t size;
	unsigned char pointer[];
};

struct lone_values {
	struct lone_values *next;
	struct lone_value value;
};

struct lone_lisp {
	struct lone_memory *memory;
	struct lone_values *values;
};

static void lone_lisp_initialize(struct lone_lisp *lone, unsigned char *memory, size_t size)
{
	lone->memory = (struct lone_memory *) memory;
	lone->memory->prev = lone->memory->next = 0;
	lone->memory->free = 1;
	lone->memory->size = size - sizeof(struct lone_memory);
	lone->values = 0;
}

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

static void *lone_allocate(struct lone_lisp *lone, size_t requested_size)
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

static void lone_deallocate_all(struct lone_lisp *lone)
{
	struct lone_values *value = lone->values, *next;

	while (value) {
		next = value->next;

		switch (value->value.type) {
		case LONE_SYMBOL:
		case LONE_TEXT:
		case LONE_BYTES:
			lone_deallocate(lone, value->value.bytes.pointer);
			break;
		}

		lone_deallocate(lone, value);

		value = next;
	}

	lone->values = 0;
}

static void *lone_reallocate(struct lone_lisp *lone, void *pointer, size_t size)
{
	struct lone_memory *old = ((struct lone_memory *) pointer) - 1,
	                   *new = ((struct lone_memory *) lone_allocate(lone, size)) - 1;

	if (pointer) {
		lone_memory_move(old->pointer, new->pointer, new->size < old->size ? new->size : old->size);
		lone_deallocate(lone, pointer);
	}

	return new->pointer;
}

static struct lone_value *lone_value_create(struct lone_lisp *lone)
{
	struct lone_values *values = lone_allocate(lone, sizeof(struct lone_values));
	if (lone->values) { values->next = lone->values; }
	lone->values = values;
	return &values->value;
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

static struct lone_value *lone_table_create(struct lone_lisp *lone, size_t capacity)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_TABLE;
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
	return length;
}

static struct lone_value *lone_text_create_from_c_string(struct lone_lisp *lone, char *c_string)
{
	return lone_text_create(lone, (unsigned char *) c_string, lone_c_string_length(c_string) - 1);
}

static struct lone_value *lone_symbol_create(struct lone_lisp *lone, unsigned char *text, size_t length)
{
	struct lone_value *value = lone_bytes_create(lone, text, length);
	value->type = LONE_SYMBOL;
	return value;
}

static struct lone_value *lone_symbol_create_from_c_string(struct lone_lisp *lone, char *c_string)
{
	return lone_symbol_create(lone, (unsigned char*) c_string, lone_c_string_length(c_string) - 1);
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

static struct lone_value *lone_list_pop(struct lone_value **list)
{
	struct lone_value *value = (*list)->list.first;
	*list = (*list)->list.rest;
	return value;
}

static int lone_bytes_equals(struct lone_bytes x, struct lone_bytes y)
{
	if (x.count != y.count) return 0;
	for (size_t i = 0; i < x.count; ++i) if (x.pointer[i] != y.pointer[i]) return 0;
	return 1;
}

static unsigned long fnv_1a(unsigned char *bytes, size_t count)
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

/* ╭──────────────────────────┨ LONE LISP LEXER ┠───────────────────────────╮
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
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_lexer {
	struct lone_bytes input;
	size_t position;
};

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The peek(k) function returns the k-th element from the input        │
   │    starting from the current input position, with peek(0) being        │
   │    the current character and peek(k) being look ahead for k > 1.       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static unsigned char *lone_lexer_peek_k(struct lone_lexer *lexer, size_t k)
{
	if (lexer->position + k >= lexer->input.count)
		return 0;
	return lexer->input.pointer + lexer->position + k;
}

static unsigned char *lone_lexer_peek(struct lone_lexer *lexer)
{
	return lone_lexer_peek_k(lexer, 0);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The consume(k) function advances the input position by k.           │
   │    This progresses through the input, consuming it.                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static void lone_lexer_consume_k(struct lone_lexer *lexer, size_t k)
{
	lexer->position += k;
}

static void lone_lexer_consume(struct lone_lexer *lexer)
{
	lone_lexer_consume_k(lexer, 1);
}

static int lone_lexer_match_byte(unsigned char byte, unsigned char target)
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
static int lone_lexer_consume_number(struct lone_lisp *lone, struct lone_lexer *lexer, struct lone_value *list)
{
	unsigned char *current, *start = lone_lexer_peek(lexer);
	if (!start) { return 1; }
	size_t end = 0;

	switch (*start) {
	case '+': case '-':
		lone_lexer_consume(lexer);
		++end;
		break;
	default:
		break;
	}

	if ((current = lone_lexer_peek(lexer)) && lone_lexer_match_byte(*current, '1')) {
		lone_lexer_consume(lexer);
		++end;
	} else { return 1; }

	while ((current = lone_lexer_peek(lexer)) && lone_lexer_match_byte(*current, '1')) {
		lone_lexer_consume(lexer);
		++end;
	}

	if (current && *current != ')' && !lone_lexer_match_byte(*current, ' ')) { return 1; }

	lone_list_set(list, lone_integer_parse(lone, start, end));
	return 0;

}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a symbol and adds it to the tokens list if valid.          │
   │                                                                        │
   │    (.*)[) \n\t]                                                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static int lone_lexer_consume_symbol(struct lone_lisp *lone, struct lone_lexer *lexer, struct lone_value *list)
{
	unsigned char *current, *start = lone_lexer_peek(lexer);
	if (!start) { return 1; }
	size_t end = 0;

	while ((current = lone_lexer_peek(lexer)) && *current != ')' && !lone_lexer_match_byte(*current, ' ')) {
		lone_lexer_consume(lexer);
		++end;
	}

	lone_list_set(list, lone_symbol_create(lone, start, end));
	return 0;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a string and adds it to the tokens list if valid.          │
   │                                                                        │
   │    (".*")[) \n\t]                                                      │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static int lone_lexer_consume_text(struct lone_lisp *lone, struct lone_lexer *lexer, struct lone_value *list)
{
	size_t end = 0;
	unsigned char *current, *start = lone_lexer_peek(lexer);
	if (!start || *start != '"') { return 1; }

	// skip leading "
	++start;
	lone_lexer_consume(lexer);

	while ((current = lone_lexer_peek(lexer)) && *current != '"') {
		lone_lexer_consume(lexer);
		++end;
	}

	// skip trailing "
	++current;
	lone_lexer_consume(lexer);

	if (*current != ')' && !lone_lexer_match_byte(*current, ' ')) { return 1; }

	lone_list_set(list, lone_text_create(lone, start, end));
	return 0;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes opening and closing parentheses                            │
   │    and adds them to the tokens list if valid.                          │
   │                                                                        │
   │    ([()])                                                              │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static int lone_lexer_consume_parenthesis(struct lone_lisp *lone, struct lone_lexer *lexer, struct lone_value *list)
{
	unsigned char *parenthesis = lone_lexer_peek(lexer);
	if (!parenthesis) { return 1; }

	switch (*parenthesis) {
	case '(': case ')':
		lone_list_set(list, lone_symbol_create(lone, parenthesis, 1));
		lone_lexer_consume(lexer);
		break;
	default: return 1;
	}

	return 0;
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
static struct lone_value *lone_lex(struct lone_lisp *lone, struct lone_value *value)
{
	struct lone_value *first = lone_list_create_nil(lone), *current = first;
	struct lone_lexer lexer = { value->bytes, 0 };
	unsigned char *c;

	while ((c = lone_lexer_peek(&lexer))) {
		if (lone_lexer_match_byte(*c, ' ')) {
			lone_lexer_consume(&lexer);
			continue;
		} else {
			unsigned char *c1;
			int failed = 1;

			switch (*c) {
			case '+': case '-':
				if ((c1 = lone_lexer_peek_k(&lexer, 1)) && lone_lexer_match_byte(*c1, '1')) {
					failed = lone_lexer_consume_number(lone, &lexer, current);
				} else {
					failed = lone_lexer_consume_symbol(lone, &lexer, current);
				}
				break;
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				failed = lone_lexer_consume_number(lone, &lexer, current);
				break;
			case '"':
				failed = lone_lexer_consume_text(lone, &lexer, current);
				break;
			case '(':
			case ')':
				failed = lone_lexer_consume_parenthesis(lone, &lexer, current);
				break;
			default:
				failed = lone_lexer_consume_symbol(lone, &lexer, current);
				break;
			}

			if (failed) {
				goto lex_failed;
			}

			current = lone_list_append(current, lone_list_create_nil(lone));
		}
	}

	return first;

lex_failed:
	linux_exit(-1);
}

static struct lone_value *lone_parse_tokens(struct lone_lisp *, struct lone_value **);

static struct lone_value *lone_parse_list(struct lone_lisp *lone, struct lone_value **tokens)
{
	struct lone_value *list = lone_list_create_nil(lone), *first = list;

	while (1) {
		if (lone_is_nil(*tokens)) {
			// expected token or ) but found end of input
			linux_exit(-1);
		}

		struct lone_value *current = (*tokens)->list.first;

		if (current->type == LONE_SYMBOL && *current->bytes.pointer == ')') {
			lone_list_pop(tokens);
			break;
		}

		lone_list_set(list, lone_parse_tokens(lone, tokens));
		list = lone_list_append(list, lone_list_create_nil(lone));
	}

	return first;
}

static struct lone_value *lone_parse_tokens(struct lone_lisp *lone, struct lone_value **tokens)
{
	if ((*tokens)->list.first == 0) { goto parse_failed; }
	struct lone_value *token = lone_list_pop(tokens);

	/* lexer may already have parsed some types */
	switch (token->type) {
	case LONE_SYMBOL:
		switch (*token->bytes.pointer) {
		case '(':
			return lone_parse_list(lone, tokens);
		case ')':
			goto parse_failed;
		default:
			return token;
		}
	case LONE_INTEGER:
	case LONE_TEXT:
		return token;
	default:
		linux_exit(-1);
	}

parse_failed:
	linux_exit(-1);
}

static struct lone_value *lone_parse(struct lone_lisp *lone, struct lone_value *value)
{
	struct lone_value *tokens = lone_lex(lone, value);
	return lone_parse_tokens(lone, &tokens);
}

static struct lone_value *lone_read_all_input(struct lone_lisp *lone, int fd)
{
	#define LONE_BUFFER_SIZE 4096
	size_t size = LONE_BUFFER_SIZE;
	unsigned char *buffer = lone_allocate(lone, size);
	ssize_t bytes_read = 0, total_read = 0;
	struct lone_value *input;

	while (1) {
		bytes_read = linux_read(fd, buffer + total_read, LONE_BUFFER_SIZE);

		if (bytes_read < 0) {
			linux_exit(-1);
		}

		total_read += bytes_read;

		if (bytes_read == LONE_BUFFER_SIZE) {
			size += LONE_BUFFER_SIZE;
			buffer = lone_reallocate(lone, buffer, size);
		} else {
			break;
		}
	}

	input = lone_bytes_create(lone, buffer, (size_t) total_read);
	lone_deallocate(lone, buffer);
	return input;
}

static struct lone_value *lone_read(struct lone_lisp *lone, int fd)
{
	return lone_parse(lone, lone_read_all_input(lone, fd));
}

static struct lone_value *lone_evaluate(struct lone_lisp *lone, struct lone_value *value)
{
	switch (value->type) {
	case LONE_BYTES:
	case LONE_LIST:
	case LONE_TABLE:
	case LONE_INTEGER:
	case LONE_POINTER:
	case LONE_TEXT:
	case LONE_SYMBOL:
		return value;
		break;
	}
}

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

static void lone_print(struct lone_lisp *, struct lone_value *, int);

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

static void lone_print(struct lone_lisp *lone, struct lone_value *value, int fd)
{
	if (value == 0 || lone_is_nil(value)) { return; }

	switch (value->type) {
	case LONE_LIST:
		linux_write(fd, "(", 1);
		lone_print_list(lone, value, fd);
		linux_write(fd, ")", 1);
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
struct auxiliary {
	long type;
	union {
		char *c_string;
		void *pointer;
		long integer;
	} as;
};

static struct lone_value *lone_create_auxiliary_value_pair(struct lone_lisp *lone, struct auxiliary *auxiliary_value)
{
	struct lone_value *key, *value;
	switch (auxiliary_value->type) {
	case AT_BASE_PLATFORM:
		key = lone_symbol_create_from_c_string(lone, "base-platform");
		value = lone_text_create_from_c_string(lone, auxiliary_value->as.c_string);
		break;
	case AT_PLATFORM:
		key = lone_symbol_create_from_c_string(lone, "platform");
		value = lone_text_create_from_c_string(lone, auxiliary_value->as.c_string);
		break;
	case AT_HWCAP:
		key = lone_symbol_create_from_c_string(lone, "hardware-capabilities");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_HWCAP2:
		key = lone_symbol_create_from_c_string(lone, "hardware-capabilities-2");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_FLAGS:
		key = lone_symbol_create_from_c_string(lone, "flags");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_NOTELF:
		key = lone_symbol_create_from_c_string(lone, "not-ELF");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_BASE:
		key = lone_symbol_create_from_c_string(lone, "interpreter-base-address");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer);
		break;
	case AT_ENTRY:
		key = lone_symbol_create_from_c_string(lone, "entry-point");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer);
		break;
	case AT_SYSINFO_EHDR:
		key = lone_symbol_create_from_c_string(lone, "vDSO");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer);
		break;
	case AT_PHDR:
		key = lone_symbol_create_from_c_string(lone, "program-headers-address");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer);
		break;
	case AT_PHENT:
		key = lone_symbol_create_from_c_string(lone, "program-headers-entry-size");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_PHNUM:
		key = lone_symbol_create_from_c_string(lone, "program-headers-count");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_EXECFN:
		key = lone_symbol_create_from_c_string(lone, "executable-file-name");
		value = lone_text_create_from_c_string(lone, auxiliary_value->as.c_string);
		break;
	case AT_EXECFD:
		key = lone_symbol_create_from_c_string(lone, "executable-file-descriptor");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_UID:
		key = lone_symbol_create_from_c_string(lone, "user-id");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_EUID:
		key = lone_symbol_create_from_c_string(lone, "effective-user-id");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_GID:
		key = lone_symbol_create_from_c_string(lone, "group-id");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_EGID:
		key = lone_symbol_create_from_c_string(lone, "effective-group-id");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_PAGESZ:
		key = lone_symbol_create_from_c_string(lone, "page-size");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
#ifdef AT_MINSIGSTKSZ
	case AT_MINSIGSTKSZ:
		key = lone_symbol_create_from_c_string(lone, "minimum-signal-delivery-stack-size");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
#endif
	case AT_CLKTCK:
		key = lone_symbol_create_from_c_string(lone, "clock-tick");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	case AT_RANDOM:
		key = lone_symbol_create_from_c_string(lone, "random");
		value = lone_bytes_create(lone, auxiliary_value->as.pointer, 16);
		break;
	case AT_SECURE:
		key = lone_symbol_create_from_c_string(lone, "secure");
		value = lone_integer_create(lone, auxiliary_value->as.integer);
		break;
	default:
		key = lone_symbol_create_from_c_string(lone, "unknown");
		value = lone_list_create(lone,
		                         lone_integer_create(lone, auxiliary_value->type),
		                         lone_integer_create(lone, auxiliary_value->as.integer));
	}
	return lone_list_create(lone, key, value);
}

static struct lone_value *lone_split_key_value_on(struct lone_lisp *lone, char *c_string, char delimiter)
{
	char *key = c_string, *value = 0;

	while (*c_string++) {
		if (*c_string == delimiter) {
			*c_string = '\0';
			value = c_string + 1;
			break;
		}
	}

	return lone_list_create(lone, lone_text_create_from_c_string(lone, key), lone_text_create_from_c_string(lone, value));
}

long lone(int argc, char **argv, char **envp, struct auxiliary *auxval)
{
	#define LONE_MEMORY_SIZE 65536
	static unsigned char memory[LONE_MEMORY_SIZE];
	struct lone_lisp lone;

	lone_lisp_initialize(&lone, memory, sizeof(memory));

	struct lone_value *arguments = lone_list_create_nil(&lone);
	struct lone_value *environment = lone_list_create_nil(&lone);
	struct lone_value *auxiliary_values = lone_list_create_nil(&lone);
	struct lone_value *head;
	int i;

	for (i = 0, head = arguments; i < argc; ++i) {
		lone_list_set(head, lone_text_create_from_c_string(&lone, argv[i]));
		head = lone_list_append(head, lone_list_create_nil(&lone));
	}

	for (i = 0, head = environment; envp[i]; ++i) {
		lone_list_set(head, lone_split_key_value_on(&lone, envp[i], '='));
		head = lone_list_append(head, lone_list_create_nil(&lone));
	}

	for (head = auxiliary_values; auxval->type != AT_NULL; ++auxval) {
		lone_list_set(head, lone_create_auxiliary_value_pair(&lone, auxval));
		head = lone_list_append(head, lone_list_create_nil(&lone));
	}

	linux_write(1, "Arguments: ", sizeof("Arguments: ") - 1);
	lone_print(&lone, arguments, 1);
	linux_write(1, "\n\n", 2);

	linux_write(1, "Environment: ", sizeof("Environment: ") - 1);
	lone_print(&lone, environment, 1);
	linux_write(1, "\n\n", 2);

	linux_write(1, "Auxiliary values: ", sizeof("Auxiliary values: ") - 1);
	lone_print(&lone, auxiliary_values, 1);
	linux_write(1, "\n\n", 2);

	lone_print(&lone, lone_evaluate(&lone, lone_read(&lone, 0)), 1);
	linux_write(1, "\n", 1);

	lone_deallocate_all(&lone);

	return 0;
}
