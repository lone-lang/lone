/* SPDX-License-Identifier: AGPL-3.0-or-later */

/* ╭─────────────────────────────┨ LONE LISP ┠──────────────────────────────╮
   │                                                                        │
   │                       The standalone Linux Lisp                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#include <stdint.h>

#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/structures.h>
#include <lone/hash.h>
#include <lone/value.h>
#include <lone/value/module.h>
#include <lone/value/function.h>
#include <lone/value/primitive.h>
#include <lone/value/bytes.h>
#include <lone/value/text.h>
#include <lone/value/symbol.h>
#include <lone/value/list.h>
#include <lone/value/vector.h>
#include <lone/value/table.h>
#include <lone/value/integer.h>
#include <lone/value/pointer.h>
#include <lone/memory.h>
#include <lone/linux.h>
#include <lone/lisp.h>

static struct lone_value *lone_intern(struct lone_lisp *lone, unsigned char *bytes, size_t count, bool should_deallocate)
{
	struct lone_value *key, *value;

	key = should_deallocate? lone_symbol_create(lone, bytes, count) : lone_symbol_transfer(lone, bytes, count, should_deallocate);
	value = lone_table_get(lone, lone->symbol_table, key);

	if (lone_is_nil(value)) {
		value = key;
		lone_table_set(lone, lone->symbol_table, key, value);
	}

	return value;
}

struct lone_value *lone_intern_c_string(struct lone_lisp *lone, char *c_string)
{
	return lone_intern(lone, (unsigned char *) c_string, lone_c_string_length(c_string), false);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Pointer dereferencing functions.                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_pointer_dereference(struct lone_lisp *lone, struct lone_value *pointer)
{
	enum lone_pointer_type type;
	void *address;

	if (!lone_is_pointer(pointer)) { /* can't dereference this value */ linux_exit(-1); }

	type = pointer->pointer.type;
	address = pointer->pointer.address;

	switch (type) {
	case LONE_TO_U8:
		return lone_integer_create(lone, *((uint8_t *) address));
	case LONE_TO_I8:
		return lone_integer_create(lone, *((int8_t *) address));
	case LONE_TO_U16:
		return lone_integer_create(lone, *((uint16_t *) address));
	case LONE_TO_I16:
		return lone_integer_create(lone, *((int16_t *) address));
	case LONE_TO_U32:
		return lone_integer_create(lone, *((uint32_t *) address));
	case LONE_TO_I32:
		return lone_integer_create(lone, *((int32_t *) address));
	case LONE_TO_U64:
		return lone_integer_create(lone, (long) *((uint64_t *) address));
	case LONE_TO_I64:
		return lone_integer_create(lone, *((int64_t *) address));
	case LONE_TO_UNKNOWN:
		/* cannot dereference pointer to unknown type */ linux_exit(-1);
	}
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

static void lone_reader_finalize(struct lone_lisp *lone, struct lone_reader *reader)
{
	lone_deallocate(lone, reader->buffer.bytes.pointer);
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

	return lone_intern(lone, start, end, true);
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
	case '\'': case '`':
	case '.':
		lone_reader_consume(reader);
		return lone_intern(lone, bracket, 1, true);
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
			case '\'': case '`':
			case '.':
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
		if (lone_is_symbol(value) && *value->bytes.pointer == ']') {
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
		if (lone_is_symbol(key) && *key->bytes.pointer == '}') {
			/* complete table: {}, { x y } */
			break;
		}

		key = lone_parse(lone, reader, key);

		value = lone_lex(lone, reader);

		if (!value) { /* end of input */ reader->error = 1; return 0; }
		if (lone_is_symbol(value) && *value->bytes.pointer == '}') {
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
	struct lone_value *list = lone_list_create_nil(lone), *first = list, *prev = 0, *next;

	while (1) {
		next = lone_lex(lone, reader);
		if (!next) { reader->error = 1; return 0; }

		if (lone_is_symbol(next)) {
			if (*next->bytes.pointer == ')') { break; }
			else if (*next->bytes.pointer == '.') {
				if (!prev) { reader->error = 1; return 0; }

				next = lone_lex(lone, reader);
				if (!next) { reader->error = 1; return 0; }

				lone_list_set_rest(prev, lone_parse(lone, reader, next));

				next = lone_lex(lone, reader);
				if (!next || !lone_is_symbol(next) || *next->bytes.pointer != ')') { reader->error = 1; return 0; }

				break;
			}
		}

		prev = list;
		list = lone_list_append(lone, list, lone_parse(lone, reader, next));
	}

	return first;
}

static struct lone_value *lone_parse_special_character(struct lone_lisp *lone, struct lone_reader *reader, char character)
{
	struct lone_value *symbol, *value, *form;
	char *c_string;

	switch (character) {
	case '\'':
		c_string = "quote";
		break;
	case '`':
		c_string = "quasiquote";
		break;
	default:
		/* invalid special character */ linux_exit(-1);
	}

	symbol = lone_intern_c_string(lone, c_string);
	value = lone_parse(lone, reader, lone_lex(lone, reader));
	form = lone_list_create(lone, value, lone_nil(lone));

	return lone_list_create(lone, symbol, form);
}

static struct lone_value *lone_parse(struct lone_lisp *lone, struct lone_reader *reader, struct lone_value *token)
{
	char character;

	if (!token) { return 0; }

	// lexer has already parsed atoms
	// parser deals with nested structures
	switch (token->type) {
	case LONE_SYMBOL:
		character = *token->bytes.pointer;

		switch (character) {
		case '(':
			return lone_parse_list(lone, reader);
		case '[':
			return lone_parse_vector(lone, reader);
		case '{':
			return lone_parse_table(lone, reader);
		case ')': case ']': case '}':
			goto parse_failed;
		case '\'': case '`':
			return lone_parse_special_character(lone, reader, character);
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
	/* parse failed */ linux_exit(-1);
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
static struct lone_value *lone_evaluate(struct lone_lisp *, struct lone_value *, struct lone_value *, struct lone_value *);

static struct lone_value *lone_evaluate_module(struct lone_lisp *lone, struct lone_value *module, struct lone_value *value)
{
	return lone_evaluate(lone, module, module->module.environment, value);
}

static struct lone_value *lone_evaluate_form_index(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *collection, struct lone_value *arguments)
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
		return get(lone, collection, lone_evaluate(lone, module, environment, key));
	} else {
		/* at least one argument */
		value = lone_list_first(arguments);
		arguments = lone_list_rest(arguments);
		if (lone_is_nil(arguments)) {
			/* table set: (collection key value) */
			set(lone, collection,
			          lone_evaluate(lone, module, environment, key),
			          lone_evaluate(lone, module, environment, value));
			return value;
		} else {
			/* too many arguments given: (collection key value extra) */
			linux_exit(-1);
		}
	}
}

static struct lone_value *lone_apply(struct lone_lisp *, struct lone_value *, struct lone_value *, struct lone_value *, struct lone_value *);
static struct lone_value *lone_apply_function(struct lone_lisp *, struct lone_value *, struct lone_value *, struct lone_value *, struct lone_value *);
static struct lone_value *lone_apply_primitive(struct lone_lisp *, struct lone_value *, struct lone_value *, struct lone_value *, struct lone_value *);

static struct lone_value *lone_evaluate_form(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *list)
{
	struct lone_value *first = lone_list_first(list), *rest = lone_list_rest(list);

	/* apply arguments to a lone value */
	first = lone_evaluate(lone, module, environment, first);
	switch (first->type) {
	case LONE_FUNCTION:
	case LONE_PRIMITIVE:
		return lone_apply(lone, module, environment, first, rest);
	case LONE_VECTOR:
	case LONE_TABLE:
		return lone_evaluate_form_index(lone, module, environment, first, rest);
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

static struct lone_value *lone_evaluate(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *value)
{
	if (value == 0) { return 0; }
	if (lone_is_nil(value)) { return value; }

	switch (value->type) {
	case LONE_LIST:
		return lone_evaluate_form(lone, module, environment, value);
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

static struct lone_value *lone_evaluate_all(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *list)
{
	struct lone_value *evaluated = lone_list_create_nil(lone), *head;

	for (head = evaluated; !lone_is_nil(list); list = lone_list_rest(list)) {
		head = lone_list_append(lone, head, lone_evaluate(lone, module, environment, lone_list_first(list)));
	}

	return evaluated;
}

static struct lone_value *lone_apply(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *applicable, struct lone_value *arguments)
{
	if (!lone_is_applicable(applicable)) { /* given function is not an applicable type */ linux_exit(-1); }

	if (lone_is_function(applicable)) {
		return lone_apply_function(lone, module, environment, applicable, arguments);
	} else {
		return lone_apply_primitive(lone, module, environment, applicable, arguments);
	}
}

static struct lone_value *lone_apply_function(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *function, struct lone_value *arguments)
{
	struct lone_value *new_environment = lone_table_create(lone, 16, function->function.environment),
	                  *names = function->function.arguments, *code = function->function.code,
	                  *value = lone_nil(lone);

	if (function->function.flags.evaluate_arguments) { arguments = lone_evaluate_all(lone, module, environment, arguments); }

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
		value = lone_evaluate(lone, module, new_environment, value);
		code = lone_list_rest(code);
	}

	if (function->function.flags.evaluate_result) { value = lone_evaluate(lone, module, environment, value); }

	return value;
}

static struct lone_value *lone_apply_primitive(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *primitive, struct lone_value *arguments)
{
	struct lone_value *result;
	if (primitive->primitive.flags.evaluate_arguments) { arguments = lone_evaluate_all(lone, module, environment, arguments); }
	result = primitive->primitive.function(lone, module, environment, arguments, primitive->primitive.closure);
	if (primitive->primitive.flags.evaluate_result) { result = lone_evaluate(lone, module, environment, result); }
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

static void lone_print_pointer(struct lone_lisp *lone, struct lone_value *pointer, int fd)
{
	if (pointer->pointer.type == LONE_TO_UNKNOWN) {
		lone_print_integer(fd, (intptr_t) pointer->pointer.address);
	} else {
		lone_print(lone, lone_pointer_dereference(lone, pointer), fd);
	}
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

	if (lone_is_list(rest)) {
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
		lone_print_integer(fd, value->integer);
		break;
	case LONE_POINTER:
		lone_print_pointer(lone, value, fd);
		break;
	}
}

/* ╭───────────────────┨ LONE LISP PRIMITIVE FUNCTIONS ┠────────────────────╮
   │                                                                        │
   │    Lone lisp functions implemented in C.                               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_primitive_begin(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *value;

	for (value = lone_nil(lone); !lone_is_nil(arguments); arguments = lone_list_rest(arguments)) {
		value = lone_list_first(arguments);
		value = lone_evaluate(lone, module, environment, value);
	}

	return value;
}

static struct lone_value *lone_primitive_when(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *test;

	if (lone_is_nil(arguments)) { /* test not specified: (when) */ linux_exit(-1); }
	test = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);

	if (!lone_is_nil(lone_evaluate(lone, module, environment, test))) {
		return lone_primitive_begin(lone, module, environment, arguments, closure);
	}

	return lone_nil(lone);
}

static struct lone_value *lone_primitive_unless(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *test;

	if (lone_is_nil(arguments)) { /* test not specified: (unless) */ linux_exit(-1); }
	test = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);

	if (lone_is_nil(lone_evaluate(lone, module, environment, test))) {
		return lone_primitive_begin(lone, module, environment, arguments, closure);
	}

	return lone_nil(lone);
}

static struct lone_value *lone_primitive_if(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
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

	if (!lone_is_nil(lone_evaluate(lone, module, environment, value))) {
		return lone_evaluate(lone, module, environment, consequent);
	} else if (alternative) {
		return lone_evaluate(lone, module, environment, alternative);
	}

	return lone_nil(lone);
}

static struct lone_value *lone_primitive_let(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *bindings, *first, *second, *rest, *value, *new_environment;

	if (lone_is_nil(arguments)) { /* no variables to bind: (let) */ linux_exit(-1); }
	bindings = lone_list_first(arguments);
	if (!lone_is_list(bindings)) { /* expected list but got something else: (let 10) */ linux_exit(-1); }

	new_environment = lone_table_create(lone, 8, environment);

	while (1) {
		if (lone_is_nil(bindings)) { break; }
		first = lone_list_first(bindings);
		if (!lone_is_symbol(first)) { /* variable names must be symbols: (let ("x")) */ linux_exit(-1); }
		rest = lone_list_rest(bindings);
		if (lone_is_nil(rest)) { /* incomplete variable/value list: (let (x 10 y)) */ linux_exit(-1); }
		second = lone_list_first(rest);
		value = lone_evaluate(lone, module, new_environment, second);
		lone_table_set(lone, new_environment, first, value);
		bindings = lone_list_rest(rest);
	}

	value = lone_nil(lone);

	while (!lone_is_nil(arguments = lone_list_rest(arguments))) {
		value = lone_evaluate(lone, module, new_environment, lone_list_first(arguments));
	}

	return value;
}

static struct lone_value *lone_primitive_set(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *variable, *value;

	if (lone_is_nil(arguments)) {
		/* no variable to set: (set) */
		linux_exit(-1);
	}

	variable = lone_list_first(arguments);
	if (!lone_is_symbol(variable)) {
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

	value = lone_evaluate(lone, module, environment, value);
	lone_table_set(lone, environment, variable, value);

	return value;
}

static struct lone_value *lone_primitive_quote(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	if (!lone_is_nil(lone_list_rest(arguments))) { /* too many arguments: (quote x y) */ linux_exit(-1); }
	return lone_list_first(arguments);
}

static struct lone_value *lone_primitive_quasiquote(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *list, *head, *current, *element, *result, *first, *rest, *unquote, *splice;
	bool escaping, splicing;

	if (!lone_is_nil(lone_list_rest(arguments))) { /* too many arguments: (quasiquote x y) */ linux_exit(-1); }

	unquote = lone_intern_c_string(lone, "unquote");
	splice = lone_intern_c_string(lone, "unquote*");
	head = list = lone_list_create_nil(lone);
	arguments = lone_list_first(arguments);

	for (current = arguments; !lone_is_nil(current); current = lone_list_rest(current)) {
		element = lone_list_first(current);

		if (lone_is_list(element)) {
			first = lone_list_first(element);
			rest = lone_list_rest(element);

			if (lone_is_equivalent(first, unquote)) {
				escaping = true;
				splicing = false;
			} else if (lone_is_equivalent(first, splice)) {
				escaping = true;
				splicing = true;
			} else {
				escaping = false;
				splicing = false;
			}

			if (escaping) {
				first = lone_list_first(rest);
				rest = lone_list_rest(rest);

				if (!lone_is_nil(rest)) { /* too many arguments: (quasiquote (unquote x y) (unquote* x y)) */ linux_exit(-1); }

				result = lone_evaluate(lone, module, environment, first);

				if (splicing) {
					if (lone_is_list(result)) {
						for (/* result */; !lone_is_nil(result); result = lone_list_rest(result)) {
							head = lone_list_append(lone, head, lone_list_first(result));
						}
					} else {
						head = lone_list_append(lone, head, result);
					}

				} else {
					head = lone_list_append(lone, head, result);
				}

			} else {
				head = lone_list_append(lone, head, element);
			}

		} else {
			head = lone_list_append(lone, head, element);
		}
	}

	return list;
}

static struct lone_value *lone_primitive_lambda_with_flags(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *arguments, struct lone_function_flags flags)
{
	struct lone_value *bindings, *code;

	bindings = lone_list_first(arguments);
	if (!lone_is_list(bindings)) { /* parameters not a list: (lambda 10) */ linux_exit(-1); }

	code = lone_list_rest(arguments);

	return lone_function_create(lone, bindings, code, environment, flags);
}

static struct lone_value *lone_primitive_lambda(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_function_flags flags = {
		.evaluate_arguments = 1,
		.evaluate_result = 0,
		.variable_arguments = 0,
	};

	return lone_primitive_lambda_with_flags(lone, environment, arguments, flags);
}

static struct lone_value *lone_primitive_lambda_bang(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_function_flags flags = {
		.evaluate_arguments = 0,
		.evaluate_result = 0,
		.variable_arguments = 0,
	};

	return lone_primitive_lambda_with_flags(lone, environment, arguments, flags);
}

static struct lone_value *lone_primitive_lambda_star(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_function_flags flags = {
		.evaluate_arguments = 1,
		.evaluate_result = 0,
		.variable_arguments = 1,
	};

	return lone_primitive_lambda_with_flags(lone, environment, arguments, flags);
}

static struct lone_value *lone_apply_predicate(struct lone_lisp *lone, struct lone_value *arguments, lone_predicate function)
{
	if (lone_is_nil(arguments) || !lone_is_nil(lone_list_rest(arguments))) { /* predicates accept exactly one argument */ linux_exit(-1); }
	return function(lone_list_first(arguments)) ? lone_true(lone) : lone_nil(lone);
}

static struct lone_value *lone_primitive_is_list(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_predicate(lone, arguments, lone_is_list);
}

static struct lone_value *lone_primitive_is_vector(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_predicate(lone, arguments, lone_is_vector);
}

static struct lone_value *lone_primitive_is_table(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_predicate(lone, arguments, lone_is_table);
}

static struct lone_value *lone_primitive_is_symbol(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_predicate(lone, arguments, lone_is_symbol);
}

static struct lone_value *lone_primitive_is_text(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_predicate(lone, arguments, lone_is_text);
}

static struct lone_value *lone_primitive_is_integer(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_predicate(lone, arguments, lone_is_integer);
}

static struct lone_value *lone_apply_comparator(struct lone_lisp *lone, struct lone_value *arguments, lone_comparator function)
{
	struct lone_value *argument, *next;

	while (1) {
		if (lone_is_nil(arguments)) { break; }
		argument = lone_list_first(arguments);
		arguments = lone_list_rest(arguments);
		next = lone_list_first(arguments);

		if (next && !function(argument, next)) { return lone_nil(lone); }
	}

	return lone_true(lone);
}

static struct lone_value *lone_primitive_is_identical(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_comparator(lone, arguments, lone_is_identical);
}

static struct lone_value *lone_primitive_is_equivalent(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_comparator(lone, arguments, lone_is_equivalent);
}

static struct lone_value *lone_primitive_is_equal(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_comparator(lone, arguments, lone_is_equal);
}

static struct lone_value *lone_primitive_print(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
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
static struct lone_value *lone_primitive_integer_operation(struct lone_lisp *lone, struct lone_value *arguments, char operation, long accumulator)
{
	struct lone_value *argument;

	if (lone_is_nil(arguments)) { /* wasn't given any arguments to operate on: (+), (-), (*) */ goto return_accumulator; }

	do {
		argument = lone_list_first(arguments);
		if (!lone_is_integer(argument)) { /* argument is not a number */ linux_exit(-1); }

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

static struct lone_value *lone_primitive_add(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_primitive_integer_operation(lone, arguments, '+', 0);
}

static struct lone_value *lone_primitive_subtract(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *first;
	long accumulator;

	if (!lone_is_nil(arguments) && !lone_is_nil(lone_list_rest(arguments))) {
		/* at least two arguments, set initial value to the first argument: (- 100 58) */
		first = lone_list_first(arguments);
		if (!lone_is_integer(first)) { /* argument is not a number */ linux_exit(-1); }
		accumulator = first->integer;
		arguments = lone_list_rest(arguments);
	} else {
		accumulator = 0;
	}

	return lone_primitive_integer_operation(lone, arguments, '-', accumulator);
}

static struct lone_value *lone_primitive_multiply(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_primitive_integer_operation(lone, arguments, '*', 1);
}

static struct lone_value *lone_primitive_divide(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *dividend, *divisor;

	if (lone_is_nil(arguments)) { /* at least the dividend is required, (/) is invalid */ linux_exit(-1); }
	dividend = lone_list_first(arguments);
	if (!lone_is_integer(dividend)) { /* can't divide non-numbers: (/ "not a number") */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);

	if (lone_is_nil(arguments)) {
		/* not given a divisor, return 1/x instead: (/ 2) = 1/2 */
		return lone_integer_create(lone, 1 / dividend->integer);
	} else {
		/* (/ x a b c ...) = x / (a * b * c * ...) */
		divisor = lone_primitive_integer_operation(lone, arguments, '*', 1);
		return lone_integer_create(lone, dividend->integer / divisor->integer);
	}
}

static struct lone_value *lone_primitive_is_less_than(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_comparator(lone, arguments, lone_integer_is_less_than);
}

static struct lone_value *lone_primitive_is_less_than_or_equal_to(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_comparator(lone, arguments, lone_integer_is_less_than_or_equal_to);
}

static struct lone_value *lone_primitive_is_greater_than(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_comparator(lone, arguments, lone_integer_is_greater_than);
}

static struct lone_value *lone_primitive_is_greater_than_or_equal_to(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_apply_comparator(lone, arguments, lone_integer_is_greater_than_or_equal_to);
}

static struct lone_value *lone_primitive_sign(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *value;
	if (lone_is_nil(arguments)) { /* no arguments: (sign) */ linux_exit(-1); }
	value = lone_list_first(arguments);
	if (!lone_is_nil(lone_list_rest(arguments))) { /* too many arguments: (sign 1 2 3) */ linux_exit(-1); }

	if (lone_is_integer(value)) {
		return lone_integer_create(lone, value->integer > 0? 1 : value->integer < 0? -1 : 0);
	} else {
		linux_exit(-1);
	}
}

static struct lone_value *lone_primitive_is_zero(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *value = lone_primitive_sign(lone, module, environment, arguments, closure);
	if (lone_is_integer(value) && value->integer == 0) { return value; }
	else { return lone_nil(lone); }
}

static struct lone_value *lone_primitive_is_positive(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *value = lone_primitive_sign(lone, module, environment, arguments, closure);
	if (lone_is_integer(value) && value->integer > 0) { return value; }
	else { return lone_nil(lone); }
}

static struct lone_value *lone_primitive_is_negative(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *value = lone_primitive_sign(lone, module, environment, arguments, closure);
	if (lone_is_integer(value) && value->integer < 0) { return value; }
	else { return lone_nil(lone); }
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Text operations.                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_primitive_join(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_text_transfer_bytes(lone, lone_join(lone, lone_list_first(arguments), lone_list_rest(arguments), lone_is_text), true);
}

static struct lone_value *lone_primitive_concatenate(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_text_transfer_bytes(lone, lone_concatenate(lone, arguments, lone_is_text), true);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    List operations.                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_value *lone_primitive_construct(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *first, *rest;

	if (lone_is_nil(arguments)) { /* no arguments given: (construct) */ linux_exit(-1); }

	first = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(arguments)) { /* only one argument given: (construct first) */ linux_exit(-1); }

	rest = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* more than two arguments given: (construct first rest extra) */ linux_exit(-1); }

	return lone_list_create(lone, first, rest);
}

static struct lone_value *lone_primitive_first(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *argument;
	if (lone_is_nil(arguments)) { linux_exit(-1); }
	argument = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(argument)) { linux_exit(-1); }
	if (!lone_is_nil(arguments)) { linux_exit(-1); }
	return lone_list_first(argument);
}

static struct lone_value *lone_primitive_rest(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *argument;
	if (lone_is_nil(arguments)) { linux_exit(-1); }
	argument = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(argument)) { linux_exit(-1); }
	if (!lone_is_nil(arguments)) { linux_exit(-1); }
	return lone_list_rest(argument);
}

static struct lone_value *lone_primitive_list_map(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *function, *list, *results, *head;

	if (lone_is_nil(arguments)) { /* arguments not given */ linux_exit(-1); }
	function = lone_list_first(arguments);
	if (!lone_is_applicable(function)) { /* not given an applicable value */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);
	list = lone_list_first(arguments);
	if (!lone_is_list(list)) { /* can only map functions to lists */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* too many arguments given */ linux_exit(-1); }

	results = lone_list_create_nil(lone);

	for (head = results; !lone_is_nil(list); list = lone_list_rest(list)) {
		arguments = lone_list_create(lone, lone_list_first(list), lone_nil(lone));
		head = lone_list_append(lone, head, lone_apply(lone, module, environment, function, arguments));
	}

	return results;
}

static struct lone_value *lone_primitive_list_reduce(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *function, *list, *result;

	if (lone_is_nil(arguments)) { /* arguments not given */ linux_exit(-1); }
	function = lone_list_first(arguments);
	if (!lone_is_applicable(function)) { /* not given an applicable value */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);
	result = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	list = lone_list_first(arguments);
	if (!lone_is_list(list)) { /* can only map functions to lists */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* too many arguments given */ linux_exit(-1); }

	for (/* list */; !lone_is_nil(list); list = lone_list_rest(list)) {
		arguments = lone_list_build(lone, 2, result, lone_list_first(list));
		result = lone_apply(lone, module, environment, function, arguments);
	}

	return result;
}

static struct lone_value *lone_primitive_flatten(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	return lone_list_flatten(lone, arguments);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Module importing, exporting and loading operations.                 │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_import_specification {
	struct lone_value *module;         /* module value to import from */
	struct lone_value *symbols;        /* list of symbols to import */
	struct lone_value *environment;    /* environment to import symbols to */

	bool prefixed;                     /* whether to prefix symbols */
};

static struct lone_value *lone_prefix_module_name(struct lone_lisp *lone, struct lone_value *module, struct lone_value *symbol)
{
	struct lone_value *arguments = lone_list_flatten(lone, lone_list_build(lone, 2, module->module.name, symbol)),
	                  *dot = lone_intern_c_string(lone, ".");

	return lone_symbol_transfer_bytes(lone, lone_join(lone, dot, arguments, lone_has_bytes), true);
}

static void lone_import_specification(struct lone_lisp *lone, struct lone_import_specification *spec)
{
	size_t i;
	struct lone_value *module = spec->module, *symbols = spec->symbols, *environment = spec->environment, *exports = module->module.exports,
	                  *symbol, *value;

	/* bind either the exported or the specified symbols: (import (module)), (import (module x f)) */
	for (i = 0; i < symbols->vector.count; ++i) {
		symbol = lone_vector_get_value_at(lone, symbols, i);
		if (!lone_is_symbol(symbol)) { /* name not a symbol: (import (module 10)) */ linux_exit(-1); }

		if (symbols != exports && !lone_vector_contains(exports, symbol)) {
			/* attempt to import private symbol */ linux_exit(-1);
		}

		value = lone_table_get(lone, module->module.environment, symbol);

		if (spec->prefixed) {
			symbol = lone_prefix_module_name(lone, spec->module, symbol);
		}

		lone_table_set(lone, environment, symbol, value);
	}
}

static struct lone_value *lone_module_load(struct lone_lisp *lone, struct lone_value *name);

static void lone_primitive_import_form(struct lone_lisp *lone, struct lone_import_specification *spec, struct lone_value *argument)
{
	struct lone_value *name;

	if (lone_is_nil(argument)) { /* nothing to import: (import ()) */ linux_exit(-1); }

	switch (argument->type) {
	case LONE_SYMBOL:
		/* (import module) */
		name = argument;
		argument = lone_nil(lone);
		break;
	case LONE_LIST:
		/* (import (module)), (import (module symbol)) */
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

	spec->module = lone_module_load(lone, name);
	if (lone_is_nil(spec->module)) { /* module not found: (import non-existent), (import (non-existent)) */ linux_exit(-1); }

	spec->symbols = lone_is_nil(argument)? spec->module->module.exports : lone_list_to_vector(lone, argument);

	lone_import_specification(lone, spec);
}

struct lone_value *lone_primitive_import(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_import_specification spec;
	struct lone_value *prefixed = lone_intern_c_string(lone, "prefixed"),
	                  *unprefixed = lone_intern_c_string(lone, "unprefixed"),
	                  *argument;

	if (lone_is_nil(arguments)) { /* nothing to import: (import) */ linux_exit(-1); }

	spec.environment = environment;
	spec.prefixed = false;

	for (/* argument */; !lone_is_nil(arguments); arguments = lone_list_rest(arguments)) {
		argument = lone_list_first(arguments);
		if (lone_is_list(argument)) {
			lone_primitive_import_form(lone, &spec, argument);
		} else if (lone_is_symbol(argument)) {
			if (lone_is_equivalent(argument, prefixed)) { spec.prefixed = true; }
			else if (lone_is_equivalent(argument, unprefixed)) { spec.prefixed = false; }
		} else {
			/* invalid import argument */ linux_exit(-1);
		}
	}

	return lone_nil(lone);
}

static void lone_export(struct lone_lisp *lone, struct lone_value *module, struct lone_value *symbol)
{
	if (!lone_is_symbol(symbol)) { /* only symbols can be exported */ linux_exit(-1); }
	lone_vector_push(lone, module->module.exports, symbol);
}

static void lone_set_and_export(struct lone_lisp *lone, struct lone_value *module, struct lone_value *symbol, struct lone_value *value)
{
	lone_export(lone, module, symbol);
	lone_table_set(lone, module->module.environment, symbol, value);
}

struct lone_value *lone_primitive_export(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *closure)
{
	struct lone_value *head, *symbol;

	for (head = arguments; !lone_is_nil(head); head = lone_list_rest(head)) {
		symbol = lone_list_first(head);

		lone_export(lone, module, symbol);
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
	case LONE_POINTER: return (long) value->pointer.address;
	case LONE_BYTES: case LONE_TEXT: case LONE_SYMBOL: return (long) value->bytes.pointer;
	case LONE_PRIMITIVE: return (long) value->primitive.function;
	case LONE_FUNCTION: case LONE_LIST: case LONE_VECTOR: case LONE_TABLE: case LONE_MODULE: linux_exit(-1);
	}
}

static struct lone_value *lone_primitive_linux_system_call(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *arguments, struct lone_value *linux_system_call_table)
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

	result = linux_system_call_6(number, args[0], args[1], args[2], args[3], args[4], args[5]);

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

static struct lone_bytes lone_get_auxiliary_random_bytes(struct auxiliary *value)
{
	struct lone_bytes random = { 0, 0 };

	for (/* value */; value->type != AT_NULL; ++value) {
		if (value->type == AT_RANDOM) {
			random.pointer = value->as.pointer;
			random.count = 16;
		}
	}

	return random;
}

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
		value = lone_pointer_create(lone, auxiliary_value->as.pointer, LONE_TO_UNKNOWN);
		break;
	case AT_ENTRY:
		key = lone_intern_c_string(lone, "entry-point");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer, LONE_TO_UNKNOWN);
		break;
	case AT_SYSINFO_EHDR:
		key = lone_intern_c_string(lone, "vDSO");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer, LONE_TO_UNKNOWN);
		break;
	case AT_PHDR:
		key = lone_intern_c_string(lone, "program-headers-address");
		value = lone_pointer_create(lone, auxiliary_value->as.pointer, LONE_TO_UNKNOWN);
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
		head = lone_list_append(lone, head, lone_text_create_from_c_string(lone, c_strings[i]));
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
		#include <lone/NR.c>

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
		arguments = lone_text_transfer_bytes(lone, lone_join(lone, slash, arguments, lone_has_bytes), true);
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

	lone_reader_finalize(lone, &reader);
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
	                  *count, *arguments, *environment, *auxiliary_values,
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false, .variable_arguments = true };
	primitive = lone_primitive_create(lone, "linux_system_call", lone_primitive_linux_system_call, linux_system_call_table, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "system-call"), primitive);

	lone_set_and_export(lone, module, lone_intern_c_string(lone, "system-call-table"), linux_system_call_table);

	lone_fill_linux_system_call_table(lone, linux_system_call_table);

	count = lone_integer_create(lone, argc);
	arguments = lone_arguments_to_list(lone, argc, argv);
	environment = lone_environment_to_table(lone, envp);
	auxiliary_values = lone_auxiliary_vector_to_table(lone, auxv);

	lone_set_and_export(lone, module, lone_intern_c_string(lone, "argument-count"), count);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "arguments"), arguments);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "environment"), environment);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "auxiliary-values"), auxiliary_values);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_builtin_module_math_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_module_name_to_key(lone, lone_intern_c_string(lone, "math")),
	                  *module = lone_module_create(lone, name),
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false, .variable_arguments = true };

	primitive = lone_primitive_create(lone, "add", lone_primitive_add, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "+"), primitive);

	primitive = lone_primitive_create(lone, "subtract", lone_primitive_subtract, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "-"), primitive);

	primitive = lone_primitive_create(lone, "multiply", lone_primitive_multiply, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "*"), primitive);

	primitive = lone_primitive_create(lone, "divide", lone_primitive_divide, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "/"), primitive);

	primitive = lone_primitive_create(lone, "is_less_than", lone_primitive_is_less_than, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "<"), primitive);

	primitive = lone_primitive_create(lone, "is_less_than_or_equal_to", lone_primitive_is_less_than_or_equal_to, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "<="), primitive);

	primitive = lone_primitive_create(lone, "is_greater_than", lone_primitive_is_greater_than, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, ">"), primitive);

	primitive = lone_primitive_create(lone, "is_greater_than_or_equal_to", lone_primitive_is_greater_than_or_equal_to, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, ">="), primitive);

	primitive = lone_primitive_create(lone, "sign", lone_primitive_sign, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "sign"), primitive);

	primitive = lone_primitive_create(lone, "is_zero", lone_primitive_is_zero, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "zero?"), primitive);

	primitive = lone_primitive_create(lone, "is_positive", lone_primitive_is_positive, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "positive?"), primitive);

	primitive = lone_primitive_create(lone, "is_negative", lone_primitive_is_negative, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "negative?"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_builtin_module_text_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_module_name_to_key(lone, lone_intern_c_string(lone, "text")),
	                  *module = lone_module_create(lone, name),
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false, .variable_arguments = true };

	primitive = lone_primitive_create(lone, "join", lone_primitive_join, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "join"), primitive);

	primitive = lone_primitive_create(lone, "concatenate", lone_primitive_concatenate, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "concatenate"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_builtin_module_list_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_module_name_to_key(lone, lone_intern_c_string(lone, "list")),
	                  *module = lone_module_create(lone, name),
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false, .variable_arguments = true };

	primitive = lone_primitive_create(lone, "construct", lone_primitive_construct, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "construct"), primitive);

	primitive = lone_primitive_create(lone, "first", lone_primitive_first, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "first"), primitive);

	primitive = lone_primitive_create(lone, "rest", lone_primitive_rest, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "rest"), primitive);

	primitive = lone_primitive_create(lone, "map", lone_primitive_list_map, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "map"), primitive);

	primitive = lone_primitive_create(lone, "reduce", lone_primitive_list_reduce, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "reduce"), primitive);

	primitive = lone_primitive_create(lone, "flatten", lone_primitive_flatten, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "flatten"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_builtin_module_lone_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_module_name_to_key(lone, lone_intern_c_string(lone, "lone")),
	                  *module = lone_module_create(lone, name),
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = false, .evaluate_result = false, .variable_arguments = true };

	primitive = lone_primitive_create(lone, "begin", lone_primitive_begin, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "begin"), primitive);

	primitive = lone_primitive_create(lone, "when", lone_primitive_when, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "when"), primitive);

	primitive = lone_primitive_create(lone, "unless", lone_primitive_unless, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "unless"), primitive);

	primitive = lone_primitive_create(lone, "if", lone_primitive_if, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "if"), primitive);

	primitive = lone_primitive_create(lone, "let", lone_primitive_let, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "let"), primitive);

	primitive = lone_primitive_create(lone, "set", lone_primitive_set, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "set"), primitive);

	primitive = lone_primitive_create(lone, "quote", lone_primitive_quote, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "quote"), primitive);

	primitive = lone_primitive_create(lone, "quasiquote", lone_primitive_quasiquote, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "quasiquote"), primitive);

	primitive = lone_primitive_create(lone, "lambda", lone_primitive_lambda, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "lambda"), primitive);

	primitive = lone_primitive_create(lone, "lambda_bang", lone_primitive_lambda_bang, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "lambda!"), primitive);

	primitive = lone_primitive_create(lone, "lambda_star", lone_primitive_lambda_star, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "lambda*"), primitive);

	flags = (struct lone_function_flags) { .evaluate_arguments = true, .evaluate_result = false, .variable_arguments = true };

	primitive = lone_primitive_create(lone, "print", lone_primitive_print, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "print"), primitive);

	primitive = lone_primitive_create(lone, "is_list", lone_primitive_is_list, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "list?"), primitive);

	primitive = lone_primitive_create(lone, "is_vector", lone_primitive_is_vector, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "vector?"), primitive);

	primitive = lone_primitive_create(lone, "is_table", lone_primitive_is_table, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "table?"), primitive);

	primitive = lone_primitive_create(lone, "is_symbol", lone_primitive_is_symbol, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "symbol?"), primitive);

	primitive = lone_primitive_create(lone, "is_text", lone_primitive_is_text, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "text?"), primitive);

	primitive = lone_primitive_create(lone, "is_integer", lone_primitive_is_integer, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "integer?"), primitive);

	primitive = lone_primitive_create(lone, "is_identical", lone_primitive_is_identical, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "identical?"), primitive);

	primitive = lone_primitive_create(lone, "is_equivalent", lone_primitive_is_equivalent, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "equivalent?"), primitive);

	primitive = lone_primitive_create(lone, "is_equal", lone_primitive_is_equal, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "equal?"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

static void lone_modules_initialize(struct lone_lisp *lone, int argc, char **argv, char **envp, struct auxiliary *auxv)
{
	lone_builtin_module_linux_initialize(lone, argc, argv, envp, auxv);
	lone_builtin_module_lone_initialize(lone);
	lone_builtin_module_math_initialize(lone);
	lone_builtin_module_text_initialize(lone);
	lone_builtin_module_list_initialize(lone);

	lone_vector_push_all(lone, lone->modules.path, 4,

		lone_text_create_from_c_string(lone, "."),
		lone_text_create_from_c_string(lone, "~/.lone/modules"),
		lone_text_create_from_c_string(lone, "~/.local/lib/lone/modules"),
		lone_text_create_from_c_string(lone, "/usr/lib/lone/modules")

	);
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

#include <lone/architecture/linux/entry_point.c>

long lone(int argc, char **argv, char **envp, struct auxiliary *auxv)
{
	void *stack = __builtin_frame_address(0);
	static unsigned char __attribute__((aligned(LONE_ALIGNMENT))) bytes[LONE_MEMORY_SIZE];
	struct lone_bytes memory = { sizeof(bytes), bytes }, random = lone_get_auxiliary_random_bytes(auxv);
	struct lone_lisp lone;

	lone_lisp_initialize(&lone, memory, 1024, stack, random);
	lone_modules_initialize(&lone, argc, argv, envp, auxv);

	lone_module_load_from_file_descriptor(&lone, lone_module_null(&lone), 0);

	return 0;
}
