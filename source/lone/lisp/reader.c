/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/reader.h>

#include <lone/lisp/value/list.h>
#include <lone/lisp/value/vector.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/text.h>
#include <lone/lisp/value/symbol.h>
#include <lone/lisp/value/integer.h>

#include <lone/memory/allocator.h>

#include <lone/linux.h>

void lone_lisp_reader_for_bytes(struct lone_lisp *lone,
		struct lone_lisp_reader *reader, struct lone_bytes bytes)
{
	reader->file_descriptor = -1;
	reader->buffer.bytes = bytes;
	reader->buffer.position.read = 0;
	reader->buffer.position.write = bytes.count;
	reader->status.error = false;
	reader->status.end_of_input = false;
}

void lone_lisp_reader_for_file_descriptor(struct lone_lisp *lone,
		struct lone_lisp_reader *reader, size_t buffer_size, int file_descriptor)
{
	reader->file_descriptor = file_descriptor;
	reader->buffer.bytes.count = buffer_size;
	reader->buffer.bytes.pointer = lone_allocate(lone->system, buffer_size);
	reader->buffer.position.read = 0;
	reader->buffer.position.write = 0;
	reader->status.error = false;
	reader->status.end_of_input = false;
}

void lone_lisp_reader_finalize(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	if (reader->file_descriptor != -1) {
		lone_deallocate(lone->system, reader->buffer.bytes.pointer);
	}
}

static size_t lone_lisp_reader_fill_buffer(struct lone_lisp *lone, struct lone_lisp_reader *reader)
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
			buffer = lone_reallocate(lone->system, buffer, allocated);
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
static unsigned char *lone_lisp_reader_peek_k(struct lone_lisp *lone, struct lone_lisp_reader *reader, size_t k)
{
	size_t read_position = reader->buffer.position.read,
	       write_position = reader->buffer.position.write,
	       bytes_read;

	if (read_position + k >= write_position) {
		// we'd overrun the buffer because there's not enough input
		// fill it up by reading more first
		bytes_read = lone_lisp_reader_fill_buffer(lone, reader);
		if (bytes_read <= k) {
			// wanted at least k bytes but got less
			return 0;
		}
	}

	return reader->buffer.bytes.pointer + read_position + k;
}

static unsigned char *lone_lisp_reader_peek(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	return lone_lisp_reader_peek_k(lone, reader, 0);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The consume(k) function advances the input position by k.           │
   │    This progresses through the input, consuming it.                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static void lone_lisp_reader_consume_k(struct lone_lisp_reader *reader, size_t k)
{
	reader->buffer.position.read += k;
}

static void lone_lisp_reader_consume(struct lone_lisp_reader *reader)
{
	lone_lisp_reader_consume_k(reader, 1);
}

static int lone_lisp_reader_match_byte(unsigned char byte, unsigned char target)
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
static struct lone_lisp_value lone_lisp_reader_consume_number(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	unsigned char *start, *current;
	size_t end;

	start = lone_lisp_reader_peek(lone, reader);

	if (!start) { goto error; }

	end = 0;

	switch (*start) {
	case '+': case '-':
		lone_lisp_reader_consume(reader);
		++end;
		break;
	default:
		break;
	}

	if ((current = lone_lisp_reader_peek(lone, reader))
			&& lone_lisp_reader_match_byte(*current, '1')) {
		lone_lisp_reader_consume(reader);
		++end;
	} else {
		goto error;
	}

	while ((current = lone_lisp_reader_peek(lone, reader))
			&& lone_lisp_reader_match_byte(*current, '1')) {
		lone_lisp_reader_consume(reader);
		++end;
	}

	if (current && !lone_lisp_reader_match_byte(*current, ')')
			&& !lone_lisp_reader_match_byte(*current, ' ')) {
		goto error;
	}

	return lone_lisp_integer_parse(lone, start, end);

error:
	reader->status.error = true;
	return lone_lisp_nil();
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a symbol and adds it to the tokens list if valid.          │
   │                                                                        │
   │    (.*)[)]} \n\t]                                                      │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_lisp_value lone_lisp_reader_consume_symbol(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	unsigned char *start, *current;
	size_t end;

	start = lone_lisp_reader_peek(lone, reader);

	if (!start) { goto error; }

	end = 0;

	while ((current = lone_lisp_reader_peek(lone, reader))
			&& !lone_lisp_reader_match_byte(*current, ')')
			&& !lone_lisp_reader_match_byte(*current, ' ')) {
		lone_lisp_reader_consume(reader);
		++end;
	}

	return lone_lisp_intern(lone, start, end, true);

error:
	reader->status.error = true;
	return lone_lisp_nil();
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a string and adds it to the tokens list if valid.          │
   │                                                                        │
   │    (".*")[)]} \n\t]                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_lisp_value lone_lisp_reader_consume_text(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	unsigned char *start, *current;
	size_t end;

	start = lone_lisp_reader_peek(lone, reader);
	if (!start || *start != '"') { goto error; }

	end = 0;

	// skip leading "
	++start;
	lone_lisp_reader_consume(reader);

	while ((current = lone_lisp_reader_peek(lone, reader)) && *current != '"') {
		lone_lisp_reader_consume(reader);
		++end;
	}

	// skip trailing "
	++current;
	lone_lisp_reader_consume(reader);

	if (!lone_lisp_reader_match_byte(*current, ')') && !lone_lisp_reader_match_byte(*current, ' ')) { goto error; }

	return lone_lisp_text_copy(lone, start, end);

error:
	reader->status.error = true;
	return lone_lisp_nil();
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
static struct lone_lisp_value lone_lisp_reader_consume_character(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	unsigned char *bracket;

	bracket = lone_lisp_reader_peek(lone, reader);

	if (!bracket) { goto error; }

	switch (*bracket) {
	case '(': case ')':
	case '[': case ']':
	case '{': case '}':
	case '\'': case '`':
	case '.':
		lone_lisp_reader_consume(reader);
		return lone_lisp_intern(lone, bracket, 1, true);
	default:
		goto error;
	}

error:
	reader->status.error = true;
	return lone_lisp_nil();
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The lone lisp lexer receives as input a single lone bytes value     │
   │    containing the full source code to be processed and it outputs      │
   │    a lone list of each lisp token found in the input. For example:     │
   │                                                                        │
   │        lex ← lone_bytes = [ (abc ("zxc") ]                             │
   │        lex → lone_lisp_list  = { ( → abc → ( → "zxc" → ) }             │
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
static struct lone_lisp_value lone_lisp_lex(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	struct lone_lisp_value token;
	unsigned char *c, *c1;
	bool found;

	token = lone_lisp_nil();
	found = false;

	while ((c = lone_lisp_reader_peek(lone, reader))) {
		if (lone_lisp_reader_match_byte(*c, ' ')) {
			lone_lisp_reader_consume(reader);
			continue;
		} else {
			found = true;
			switch (*c) {
			case '+': case '-':
				if ((c1 = lone_lisp_reader_peek_k(lone, reader, 1)) && lone_lisp_reader_match_byte(*c1, '1')) {
					token = lone_lisp_reader_consume_number(lone, reader);
				} else {
					token = lone_lisp_reader_consume_symbol(lone, reader);
				}
				break;
			case '0': case '1': case '2': case '3': case '4':
			case '5': case '6': case '7': case '8': case '9':
				token = lone_lisp_reader_consume_number(lone, reader);
				break;
			case '"':
				token = lone_lisp_reader_consume_text(lone, reader);
				break;
			case '(': case ')':
			case '[': case ']':
			case '{': case '}':
			case '\'': case '`':
			case '.':
				token = lone_lisp_reader_consume_character(lone, reader);
				break;
			default:
				token = lone_lisp_reader_consume_symbol(lone, reader);
				break;
			}

			if (reader->status.error) {
				goto error;
			} else {
				break;
			}
		}
	}

	reader->status.end_of_input = !found;

	return token;

error:
	linux_exit(-1);
}

static bool lone_lisp_reader_is_expected_character_symbol(struct lone_lisp_value value, unsigned char expected)
{
	struct lone_lisp_heap_value *actual;
	unsigned char character;

	if (lone_lisp_is_symbol(value)) {
		actual = lone_lisp_heap_value_of(value);

		if (actual->as.bytes.count != 1) {
			return false;
		}

		character = *actual->as.bytes.pointer;
		return character == expected;
	} else {
		return false;
	}
}

static struct lone_lisp_value lone_lisp_parse(struct lone_lisp *, struct lone_lisp_reader *, struct lone_lisp_value);

static struct lone_lisp_value lone_lisp_parse_vector(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	struct lone_lisp_value vector, value;
	size_t i;

	vector = lone_lisp_vector_create(lone, 32);
	i = 0;

	while (1) {
		value = lone_lisp_lex(lone, reader);

		if (reader->status.end_of_input) {
			/* end of input in the middle of vector: [, [ x */
			goto error;
		}

		if (lone_lisp_reader_is_expected_character_symbol(value, ']')) {
			/* complete vector: [], [ x ], [ x y ] */
			break;
		}

		value = lone_lisp_parse(lone, reader, value);

		lone_lisp_vector_set_value_at(lone, vector, i++, value);
	}

	return vector;

error:
	reader->status.error = true;
	return lone_lisp_nil();
}

static struct lone_lisp_value lone_lisp_parse_table(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	struct lone_lisp_value table, key, value;

	table = lone_lisp_table_create(lone, 32, lone_lisp_nil());

	while (1) {
		key = lone_lisp_lex(lone, reader);

		if (reader->status.end_of_input) {
			/* end of input in the middle of table: {, { x y */
			goto error;
		}

		if (lone_lisp_reader_is_expected_character_symbol(key, '}')) {
			/* complete table: {}, { x y } */
			break;
		}

		key = lone_lisp_parse(lone, reader, key);

		value = lone_lisp_lex(lone, reader);

		if (reader->status.end_of_input) {
			/* end of input in the middle of table: { x, { x y z */
			goto error;
		}

		if (lone_lisp_reader_is_expected_character_symbol(value, '}')) {
			/* incomplete table: { x }, { x y z } */
			goto error;
		}

		value = lone_lisp_parse(lone, reader, value);

		lone_lisp_table_set(lone, table, key, value);
	}

	return table;

error:
	reader->status.error = true;
	return lone_lisp_nil();
}

static struct lone_lisp_value lone_lisp_parse_list(struct lone_lisp *lone,
		struct lone_lisp_reader *reader)
{
	struct lone_lisp_value first, head, next;
	bool at_least_one;

	first = head = lone_lisp_nil();
	at_least_one = false;

	while (1) {
		next = lone_lisp_lex(lone, reader);

		if (reader->status.end_of_input) {
			/* end of input in the middle of list: (, (x */
			goto error;
		}

		if (lone_lisp_is_symbol(next)) {
			if (lone_lisp_reader_is_expected_character_symbol(next, ')')) {

				if (at_least_one) {
					/* complete list: (1 2 3) */
					break;
				} else {
					/* empty list: () */
					return lone_lisp_nil();
				}

			} else if (lone_lisp_reader_is_expected_character_symbol(next, '.')) {

				if (!at_least_one) {
					/* pair syntax without first: ( . 2) */
					goto error;
				}

				next = lone_lisp_lex(lone, reader);

				if (reader->status.end_of_input) {
					/* end of input in the middle of pair: (1 . */
					goto error;
				}

				lone_lisp_list_set_rest(lone, head, lone_lisp_parse(lone, reader, next));

				next = lone_lisp_lex(lone, reader);

				if (!lone_lisp_reader_is_expected_character_symbol(next, ')')) {
					/* extra tokens in pair syntax: (1 2 . 3 4) */
					goto error;
				}

				break;
			}
		}

		lone_lisp_list_append(lone, &first, &head, lone_lisp_parse(lone, reader, next));
		at_least_one = true;
	}

	return first;

error:
	reader->status.error = true;
	return lone_lisp_nil();
}

static struct lone_lisp_value lone_lisp_parse_special_character(struct lone_lisp *lone,
		struct lone_lisp_reader *reader, char character)
{
	struct lone_lisp_value symbol, value;
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

	symbol = lone_lisp_intern_c_string(lone, c_string);
	value = lone_lisp_parse(lone, reader, lone_lisp_lex(lone, reader));

	return lone_lisp_list_build(lone, 2, &symbol, &value);
}

static struct lone_lisp_value lone_lisp_parse(struct lone_lisp *lone,
		struct lone_lisp_reader *reader, struct lone_lisp_value token)
{
	struct lone_lisp_heap_value *actual;
	char character;

	if (reader->status.end_of_input) { return lone_lisp_nil(); }

	/* lexer has already parsed atoms */
	switch (lone_lisp_type_of(token)) {
	case LONE_LISP_TYPE_NIL:
	case LONE_LISP_TYPE_FALSE:
	case LONE_LISP_TYPE_TRUE:
	case LONE_LISP_TYPE_INTEGER:
		return token;
	case LONE_LISP_TYPE_HEAP_VALUE:
		break;
	}

	actual = lone_lisp_heap_value_of(token);

	/* parser deals with nested structures */
	switch (actual->type) {
	case LONE_LISP_TYPE_BYTES:
	case LONE_LISP_TYPE_TEXT:
		return token;
	case LONE_LISP_TYPE_SYMBOL:

		if (actual->as.bytes.count > 1) { return token; }
		character = *actual->as.bytes.pointer;

		switch (character) {
		default:
			return token;
		case '\'': case '`':
			return lone_lisp_parse_special_character(lone, reader, character);
		case '(':
			return lone_lisp_parse_list(lone, reader);
		case '[':
			return lone_lisp_parse_vector(lone, reader);
		case '{':
			return lone_lisp_parse_table(lone, reader);
		case ')': case ']': case '}':
			/* unexpected closing bracket */
			goto error;
		}

	case LONE_LISP_TYPE_MODULE:
	case LONE_LISP_TYPE_FUNCTION:
	case LONE_LISP_TYPE_PRIMITIVE:
	case LONE_LISP_TYPE_CONTINUATION:
	case LONE_LISP_TYPE_LIST:
	case LONE_LISP_TYPE_VECTOR:
	case LONE_LISP_TYPE_TABLE:
		/* unexpected value type from lexer */
		goto error;
	}

error:
	/* parse failed */ linux_exit(-1);
}

struct lone_lisp_value lone_lisp_read(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	return lone_lisp_parse(lone, reader, lone_lisp_lex(lone, reader));
}
