/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/lisp.h>
#include <lone/lisp/reader.h>

#include <lone/value/list.h>
#include <lone/value/vector.h>
#include <lone/value/table.h>
#include <lone/value/text.h>
#include <lone/value/symbol.h>
#include <lone/value/integer.h>

#include <lone/memory/allocator.h>
#include <lone/linux.h>

#include <lone/struct/reader.h>

void lone_reader_for_bytes(struct lone_lisp *lone, struct lone_reader *reader, struct lone_bytes bytes)
{
	reader->file_descriptor = -1;
	reader->buffer.bytes = bytes;
	reader->buffer.position.read = 0;
	reader->buffer.position.write = bytes.count;
	reader->error = 0;
}

void lone_reader_for_file_descriptor(struct lone_lisp *lone, struct lone_reader *reader, size_t buffer_size, int file_descriptor)
{
	reader->file_descriptor = file_descriptor;
	reader->buffer.bytes.count = buffer_size;
	reader->buffer.bytes.pointer = lone_allocate(lone, buffer_size);
	reader->buffer.position.read = 0;
	reader->buffer.position.write = 0;
	reader->error = 0;
}

void lone_reader_finalize(struct lone_lisp *lone, struct lone_reader *reader)
{
	if (reader->file_descriptor != -1) {
		lone_deallocate(lone, reader->buffer.bytes.pointer);
	}
}

static size_t lone_reader_fill_buffer(struct lone_lisp *lone, struct lone_reader *reader)
{
	unsigned char *buffer = reader->buffer.bytes.pointer;
	size_t size = reader->buffer.bytes.count, position = reader->buffer.position.write,
	       allocated = size, bytes_read = 0, total_read = 0;
	ssize_t read_result = 0;

	if (reader->file_descriptor == -1) {
		/* reading from a fixed buffer, can't read more */
		return 0;
	}

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

struct lone_value *lone_read(struct lone_lisp *lone, struct lone_reader *reader)
{
	return lone_parse(lone, reader, lone_lex(lone, reader));
}
