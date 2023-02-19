/* SPDX-License-Identifier: AGPL-3.0-or-later */

/* ╭─────────────────────────────┨ LONE LISP ┠──────────────────────────────╮
   │                                                                        │
   │                       The standalone Linux Lisp                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#include <linux/types.h>
#include <linux/unistd.h>

typedef __kernel_size_t size_t;
typedef __kernel_ssize_t ssize_t;

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Definitions for essential Linux system calls used by lone.          │
   │    The exit system call is adorned with compiler annotations           │
   │    for better code generation.                                         │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static void __attribute__((noreturn)) linux_exit(int code)
{
	system_call_1(__NR_exit, code);
	__builtin_unreachable();
}

static ssize_t linux_read(int fd, const void *buffer, size_t count)
{
	return system_call_3(__NR_read, fd, (long) buffer, count);
}

static ssize_t linux_write(int fd, const void *buffer, size_t count)
{
	return system_call_3(__NR_write, fd, (long) buffer, count);
}

/* ╭──────────────────────────┨ LONE LISP TYPES ┠───────────────────────────╮
   │                                                                        │
   │    Lone is designed to work without any dependencies except Linux,     │
   │    so it does not make use of even the system's C library.             │
   │    In order to bootstrap itself in such harsh conditions,              │
   │    it must be given some memory to work with.                          │
   │    The lone_linux structure holds that memory.                         │
   │                                                                        │
   │    Lone implements dynamic data types as a tagged union.               │
   │    Supported types are:                                                │
   │                                                                        │
   │        ◦ Bytes    the binary data and low level string type            │
   │        ◦ List     the linked list and tree type                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_lisp {
	unsigned char *memory;
	size_t capacity;
	size_t allocated;
};

enum lone_type {
	LONE_BYTES = 0,
	LONE_LIST = 1,
	LONE_INTEGER = 2,
};

struct lone_bytes {
	size_t count;
	unsigned char *pointer;
};

struct lone_list {
	struct lone_value *first;
	struct lone_value *rest;
};

struct lone_value {
	enum lone_type type;
	union {
		struct lone_bytes bytes;
		struct lone_list list;
		unsigned long integer;
	};
};

/* ╭────────────────────┨ LONE LISP MEMORY ALLOCATION ┠─────────────────────╮
   │                                                                        │
   │    Lone will allocate from its internal memory at first.               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static void *lone_allocate(struct lone_lisp *lone, size_t size)
{
	if (lone->allocated + size > lone->capacity)
		linux_exit(-1);
	void *allocation = lone->memory + lone->allocated;
	lone->allocated += size;
	return allocation;
}

static struct lone_value *lone_value_create(struct lone_lisp *lone)
{
	return lone_allocate(lone, sizeof(struct lone_value));
}

static struct lone_value *lone_bytes_create(struct lone_lisp *lone, unsigned char *pointer, size_t count)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_BYTES;
	value->bytes.count = count;
	value->bytes.pointer = pointer;
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

static struct lone_value *lone_integer_create(struct lone_lisp *lone, long integer)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_INTEGER;
	value->integer = integer;
	return value;
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

	lone_list_set(list, lone_bytes_create(lone, start, end));
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

	lone_list_set(list, lone_bytes_create(lone, start, end));
	return 0;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a string and adds it to the tokens list if valid.          │
   │                                                                        │
   │    (".*")[) \n\t]                                                      │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static int lone_lexer_consume_bytes(struct lone_lisp *lone, struct lone_lexer *lexer, struct lone_value *list)
{
	unsigned char *current, *start = lone_lexer_peek(lexer);
	if (!start || *start != '"') { return 1; }

	size_t end = 1;
	lone_lexer_consume(lexer);

	while ((current = lone_lexer_peek(lexer)) && *current != '"') {
		lone_lexer_consume(lexer);
		++end;
	}

	// include "
	++end;
	lone_lexer_consume(lexer);

	if ((current = lone_lexer_peek(lexer)) && *current != ')' && !lone_lexer_match_byte(*current, ' ')) { return 1; }

	lone_list_set(list, lone_bytes_create(lone, start, end));
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
		lone_list_set(list, lone_bytes_create(lone, parenthesis, 1));
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
   │        lex → lone_list  = { [(] → [abc] → [(] → ["zxc"] → [)] }        │
   │                                                                        │
   │    Note that parentheses are not matched at this stage.                │
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
				failed = lone_lexer_consume_bytes(lone, &lexer, current);
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

		if (*(*tokens)->list.first->bytes.pointer == ')') {
			lone_list_pop(tokens);
			break;
		}

		lone_list_set(list, lone_parse_tokens(lone, tokens));
		list = lone_list_append(list, lone_list_create_nil(lone));
	}

	return first;
}

static struct lone_value *lone_parse_integer(struct lone_lisp *lone, struct lone_value *token)
{
	unsigned char *digits = token->bytes.pointer;
	size_t count = token->bytes.count, i = 0;
	long integer = 0;

	switch (*digits) { case '+': case '-': ++i; break; }

	while (i < count) {
		integer *= 10;
		integer += digits[i++] - '0';
	}

	if (*digits == '-') { integer *= -1; }

	return lone_integer_create(lone, integer);
}

static struct lone_value *lone_parse_atom(struct lone_lisp *lone, struct lone_value *token)
{
	switch (*token->bytes.pointer) {
	case '+': case '-':
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return lone_parse_integer(lone, token);
	default:
		return token;
	}
}

static struct lone_value *lone_parse_tokens(struct lone_lisp *lone, struct lone_value **tokens)
{
	if ((*tokens)->list.first == 0) { goto parse_failed; }
	struct lone_value *token = lone_list_pop(tokens);

	switch (*token->bytes.pointer) {
	case '(':
		return lone_parse_list(lone, tokens);
	case ')':
		goto parse_failed;
	default:
		return lone_parse_atom(lone, token);
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
	unsigned char *input = lone_allocate(lone, LONE_BUFFER_SIZE);
	ssize_t bytes_read = 0, total_read = 0;

	while (1) {
		bytes_read = linux_read(fd, input + total_read, LONE_BUFFER_SIZE);

		if (bytes_read < 0) {
			linux_exit(-1);
		}

		total_read += bytes_read;

		if (bytes_read == LONE_BUFFER_SIZE) {
			lone_allocate(lone, LONE_BUFFER_SIZE);
		} else {
			break;
		}
	}

	return lone_bytes_create(lone, input, total_read);
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
	case LONE_INTEGER:
		return value;
		break;
	}
}

static void lone_print(struct lone_lisp *lone, struct lone_value *value, int fd)
{
	if (value == 0)
		return;

	switch (value->type) {
	case LONE_LIST:
		linux_write(fd, "(", 1);
		if (value->list.first) {
			lone_print(lone, value->list.first, fd);
		}
		if (value->list.rest) {
			linux_write(fd, " . ", 3);
			lone_print(lone, value->list.rest, fd);
		}
		linux_write(fd, ")", 1);
		break;
	case LONE_BYTES:
		linux_write(fd, value->bytes.pointer, value->bytes.count);
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
#if __BITS_PER_LONG == 64
typedef __u64 auxiliary_value;
#elif __BITS_PER_LONG == 32
typedef __u32 auxiliary_value;
#else
#error "Unsupported architecture"
#endif

struct auxiliary {
	auxiliary_value type;
	auxiliary_value value;
};

long lone(int count, char **arguments, char **environment, struct auxiliary *values)
{
	#define LONE_MEMORY_SIZE 65536
	static unsigned char memory[LONE_MEMORY_SIZE];
	struct lone_lisp lone = { memory, sizeof(memory), 0 };

	lone_print(&lone, lone_evaluate(&lone, lone_read(&lone, 0)), 1);
	linux_write(1, "\n", 1);

	return 0;
}
