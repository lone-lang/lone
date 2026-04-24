/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/reader.h>

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
	reader->buffer.bytes.pointer = lone_memory_allocate(lone->system, buffer_size, 1, 1, LONE_MEMORY_ALLOCATION_FLAGS_NONE);
	reader->buffer.position.read = 0;
	reader->buffer.position.write = 0;
	reader->status.error = false;
	reader->status.end_of_input = false;
}

void lone_lisp_reader_finalize(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	if (reader->file_descriptor != -1) {
		lone_memory_deallocate(lone->system, reader->buffer.bytes.pointer, reader->buffer.bytes.count, 1, 1);
	}
}

static size_t lone_lisp_reader_fill_buffer(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	unsigned char *buffer;
	size_t allocated, position, available, bytes_read, total_read, old_allocated;
	ssize_t read_result;

	if (reader->file_descriptor == -1) {
		/* reading from a fixed buffer, can't read more */
		return 0;
	}

	buffer = reader->buffer.bytes.pointer;
	allocated = reader->buffer.bytes.count;
	position = reader->buffer.position.write;
	total_read = 0;

	while (1) {
		available = allocated - position;

		if (available == 0) {
			/* buffer is full, grow it before reading more */
			old_allocated = allocated;

			if (__builtin_mul_overflow(allocated, (size_t) 2, &allocated)) {
				goto overflow;
			}

			buffer = lone_memory_reallocate(
				lone->system, buffer,
				old_allocated, 1,
				allocated, 1,
				1,
				LONE_MEMORY_ALLOCATION_FLAGS_NONE
			);

			available = allocated - position;
		}

		read_result = linux_read(reader->file_descriptor, buffer + position, available);

		if (read_result < 0) {
			linux_exit(-1);
		}

		bytes_read  = (size_t) read_result;
		total_read += bytes_read;
		position   += bytes_read;

		if (bytes_read < available) {
			/* short read: end of input or no more data available right now */
			break;
		}
		/* full read: loop to grow and read more */
	}

	reader->buffer.bytes.pointer = buffer;
	reader->buffer.bytes.count = allocated;
	reader->buffer.position.write = position;
	return total_read;

overflow:
	linux_exit(-1);
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
	if (reader->buffer.position.read + k >= reader->buffer.position.write) {
		/* we'd overrun the buffer because there's not enough input
		 * fill it up by reading more first */
		lone_lisp_reader_fill_buffer(lone, reader);
		if (reader->buffer.position.read + k >= reader->buffer.position.write) {
			/* wanted at least k bytes but got less */
			return 0;
		}
	}

	return reader->buffer.bytes.pointer + reader->buffer.position.read + k;
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

static inline bool lone_lisp_reader_is_whitespace(unsigned char character)
{
	switch (character) {
	case ' ': case '\t': case '\n': return true;
	default:                        return false;
	}
}

static inline bool lone_lisp_reader_is_digit(unsigned char character)
{
	return character >= '0' && character <= '9';
}

static inline bool lone_lisp_reader_hex_digit_value(unsigned char character, unsigned char *value)
{
	if (character >= '0' && character <= '9') { *value = character - '0';      return true; }
	if (character >= 'a' && character <= 'f') { *value = character - 'a' + 10; return true; }
	if (character >= 'A' && character <= 'F') { *value = character - 'A' + 10; return true; }
	return false;
}

static inline bool lone_lisp_reader_is_opening_bracket(unsigned char character)
{
	switch (character) {
	case '(': case '[': case '{': return true;
	default:                      return false;
	}
}

static inline bool lone_lisp_reader_is_closing_bracket(unsigned char character)
{
	switch (character) {
	case ')': case ']': case '}': return true;
	default:                      return false;
	}
}

static inline bool lone_lisp_reader_is_token_separator(unsigned char character)
{
	return    lone_lisp_reader_is_whitespace(character)
	       || lone_lisp_reader_is_closing_bracket(character)
	       || character == ';';
}

static inline bool lone_lisp_reader_is_symbol_terminator(unsigned char character)
{
	return    lone_lisp_reader_is_token_separator(character)
	       || lone_lisp_reader_is_opening_bracket(character)
	       || character == '"';
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a number and adds it to the tokens list if valid.          │
   │                                                                        │
   │    ([+-]?[0-9]+)[)}\]; \t\n]                                           │
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

	if ((current = lone_lisp_reader_peek(lone, reader)) && lone_lisp_reader_is_digit(*current)) {
		lone_lisp_reader_consume(reader);
		++end;
	} else {
		goto error;
	}

	while ((current = lone_lisp_reader_peek(lone, reader)) && lone_lisp_reader_is_digit(*current)) {
		lone_lisp_reader_consume(reader);
		++end;
	}

	if (current && !lone_lisp_reader_is_token_separator(*current)) { goto error; }

	return lone_lisp_integer_parse(lone, start, end);

error:
	reader->status.error = true;
	return lone_lisp_nil();
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a symbol and adds it to the tokens list if valid.          │
   │                                                                        │
   │    [^ \t\n()[\]{};"]+[)}\] \t\n]                                       │
   │                                                                        │
   │    The grammar for symbols is stricter because it is hard              │
   │    to visually distinguish symbols such as symbol;comment.             │
   │    The eye cannot immediately understand whether ;comment              │
   │    is part of the symbol;comment symbol or just a comment.             │
   │    So I decided to require spaces after symbols before                 │
   │    comments can be written: symbol ;comment. This makes it             │
   │    clear where the symbol ends.                                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static struct lone_lisp_value lone_lisp_reader_consume_symbol(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	unsigned char *start, *current;
	size_t end;

	start = lone_lisp_reader_peek(lone, reader);

	if (!start) { goto error; }

	end = 0;

	while ((current = lone_lisp_reader_peek(lone, reader)) &&
	       !lone_lisp_reader_is_symbol_terminator(*current)) {

		lone_lisp_reader_consume(reader);
		++end;
	}

	if (end == 0) { /* zero length symbols shouldn't occur normally */ goto error; }

	/* symbols must be followed by space, a closing bracket or end of input
	 * they have no inherent visual delimiter so the grammar is stricter */
	if (current && !lone_lisp_reader_is_whitespace(*current)
	            && !lone_lisp_reader_is_closing_bracket(*current)) { goto error; }

	return lone_lisp_intern(lone, start, end, true);

error:
	reader->status.error = true;
	return lone_lisp_nil();
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Analyzes a string and adds it to the tokens list if valid.          │
   │                                                                        │
   │    Supported escape sequences:                                         │
   │        ◦ \\                                                            │
   │        ◦ \"                                                            │
   │        ◦ \n                                                            │
   │        ◦ \t                                                            │
   │        ◦ \0                                                            │
   │                                                                        │
   │    ("(\\[\\\"nt0]|[^"\\])*")[)}\]; \t\n]                               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

/* Consume escaped content terminated by a double quote.
 * Caller must have already consumed the opening ".
 * On success, the trailing " is consumed and the result
 * is returned in a bytes structure. The result is dynamically
 * allocated and the caller takes ownership of it.
 * On failure, returns null bytes and sets reader->status.error.
 * NUL terminator is added for C string compatibility.
 * The NUL terminator is also required for the transfer functions.
 */
static struct lone_bytes lone_lisp_reader_consume_escaped_content(
		struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	unsigned char *current, *output, character;
	size_t input_length, output_length, escapes, buffer_size;
	struct lone_bytes result = { .pointer = 0, .count = 0 };

	/* determine input size before allocating buffer */
	input_length = 0;
	escapes = 0;
	while ((current = lone_lisp_reader_peek_k(lone, reader, input_length)) &&
	       *current != '"') {

		++input_length;
		if (*current == '\\') {
			unsigned char *next;
			if (!(next = lone_lisp_reader_peek_k(lone, reader, input_length))) {
				/* backslash before end of input */ goto error;
			}
			++input_length;
			++escapes;

			if (*next == 'x') {
				/* \xNN consumes two additional hex digits */
				if (!lone_lisp_reader_peek_k(lone, reader, input_length) ||
				    !lone_lisp_reader_peek_k(lone, reader, input_length + 1)) {
					goto error;
				}
				input_length += 2;
				escapes += 2;
			}
		}
	}

	if (!current) { /* unterminated string: no trailing " */ goto error; }

	/* escape sequences squeeze two characters into one
	 * also don't forget the hidden null terminator */
	buffer_size = input_length - escapes + 1;
	output = lone_memory_allocate(lone->system, buffer_size, 1, 1, LONE_MEMORY_ALLOCATION_FLAGS_NONE);
	output_length = 0;

	/* consume input and process escape sequences */
	while ((current = lone_lisp_reader_peek(lone, reader)) && *current != '"') {
		if (*current == '\\') {
			lone_lisp_reader_consume(reader);
			current = lone_lisp_reader_peek(lone, reader);
			/* current cannot be null */

			switch (*current) {
			case '\\': character = '\\'; break;
			case '"':  character = '"';  break;
			case 'n':  character = '\n'; break;
			case 't':  character = '\t'; break;
			case '0':  character = '\0'; break;
			case 'x': {
				unsigned char hi, lo;
				/* consume 'x', peek high nibble */
				lone_lisp_reader_consume(reader);
				current = lone_lisp_reader_peek(lone, reader);
				if (!current || !lone_lisp_reader_hex_digit_value(*current, &hi)) {
					goto deallocate_and_error;
				}
				/* consume high nibble, peek low nibble */
				lone_lisp_reader_consume(reader);
				current = lone_lisp_reader_peek(lone, reader);
				if (!current || !lone_lisp_reader_hex_digit_value(*current, &lo)) {
					goto deallocate_and_error;
				}
				character = (hi << 4) | lo;
				break;
			}
			default:   goto deallocate_and_error; /* unknown escape sequence */
			}
		} else {
			character = *current;
		}

		output[output_length++] = character;
		lone_lisp_reader_consume(reader);
	}

	/* null terminate for C string compatibility */
	output[output_length] = '\0';

	/* consume trailing " */
	lone_lisp_reader_consume(reader);

	result.pointer = output;
	result.count = output_length;
	return result;

deallocate_and_error:
	lone_memory_deallocate(lone->system, output, buffer_size, 1, 1);
error:
	reader->status.error = true;
	return result;
}

static struct lone_lisp_value lone_lisp_reader_consume_text(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	unsigned char *current;
	struct lone_bytes content;
	struct lone_lisp_value value;

	current = lone_lisp_reader_peek(lone, reader);
	if (!current || *current != '"') { goto error; }

	/* skip leading " */
	lone_lisp_reader_consume(reader);

	content = lone_lisp_reader_consume_escaped_content(lone, reader);
	if (!content.pointer) { goto error; }

	/* text must be followed by a delimiter, space, comment or the end of input */
	current = lone_lisp_reader_peek(lone, reader);
	if (current && !lone_lisp_reader_is_token_separator(*current)) { goto deallocate_and_error; }

	if (content.count <= LONE_LISP_INLINE_MAX_LENGTH) {
		value = lone_lisp_inline_text_create(content.pointer, content.count);
		lone_memory_deallocate(lone->system, content.pointer, content.count + 1, 1, 1);
		return value;
	}

	return lone_lisp_text_transfer(lone, content.pointer, content.count, true);

deallocate_and_error:
	lone_memory_deallocate(lone->system, content.pointer, content.count + 1, 1, 1);
error:
	reader->status.error = true;
	return lone_lisp_nil();
}

static struct lone_lisp_value lone_lisp_reader_consume_byte_literal(struct lone_lisp *lone, struct lone_lisp_reader *reader)
{
	unsigned char *current;
	struct lone_bytes content;
	struct lone_lisp_value value;

	current = lone_lisp_reader_peek(lone, reader);
	if (!current || *current != '"') { goto error; }

	/* skip leading " */
	lone_lisp_reader_consume(reader);

	content = lone_lisp_reader_consume_escaped_content(lone, reader);
	if (!content.pointer) { goto error; }

	/* byte literal must be followed by a delimiter, space, comment or the end of input */
	current = lone_lisp_reader_peek(lone, reader);
	if (current && !lone_lisp_reader_is_token_separator(*current)) { goto deallocate_and_error; }

	if (content.count <= LONE_LISP_INLINE_MAX_LENGTH) {
		value = lone_lisp_inline_bytes_create(content.pointer, content.count);
		lone_memory_deallocate(lone->system, content.pointer, content.count + 1, 1, 1);
		return value;
	}

	value = lone_lisp_bytes_transfer(lone, content.pointer, content.count, true);
	lone_lisp_heap_value_of(lone, value)->frozen = true;
	return value;

deallocate_and_error:
	lone_memory_deallocate(lone->system, content.pointer, content.count + 1, 1, 1);
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
   │        ◦ If found ; skip everything until end of line/input            │
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
		if (lone_lisp_reader_is_whitespace(*c)) {
			lone_lisp_reader_consume(reader);
			continue;
		} else {
			found = true;
			switch (*c) {
			case '+': case '-':
				if ((c1 = lone_lisp_reader_peek_k(lone, reader, 1)) && lone_lisp_reader_is_digit(*c1)) {
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
			case ';':
				/* comment: skip until end of line or end of input */
				while ((c = lone_lisp_reader_peek(lone, reader)) && *c != '\n') {
					lone_lisp_reader_consume(reader);
				}
				found = false;
				continue;
			case 'b':
				if ((c1 = lone_lisp_reader_peek_k(lone, reader, 1)) && *c1 == '"') {
					lone_lisp_reader_consume(reader); /* consume 'b' */
					token = lone_lisp_reader_consume_byte_literal(lone, reader);
				} else {
					token = lone_lisp_reader_consume_symbol(lone, reader);
				}
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

static bool lone_lisp_reader_is_expected_character_symbol(struct lone_lisp *lone,
		struct lone_lisp_value value, unsigned char expected)
{
	struct lone_bytes name;

	if (lone_lisp_is_symbol(lone, value)) {
		name = lone_lisp_bytes_of(lone, &value);

		if (name.count != 1) {
			return false;
		}

		return *name.pointer == expected;
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

		if (lone_lisp_reader_is_expected_character_symbol(lone, value, ']')) {
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

		if (lone_lisp_reader_is_expected_character_symbol(lone, key, '}')) {
			/* complete table: {}, { x y } */
			break;
		}

		key = lone_lisp_parse(lone, reader, key);

		value = lone_lisp_lex(lone, reader);

		if (reader->status.end_of_input) {
			/* end of input in the middle of table: { x, { x y z */
			goto error;
		}

		if (lone_lisp_reader_is_expected_character_symbol(lone, value, '}')) {
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

		if (lone_lisp_is_symbol(lone, next)) {
			if (lone_lisp_reader_is_expected_character_symbol(lone, next, ')')) {

				if (at_least_one) {
					/* complete list: (1 2 3) */
					break;
				} else {
					/* empty list: () */
					return lone_lisp_nil();
				}

			} else if (lone_lisp_reader_is_expected_character_symbol(lone, next, '.')) {

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

				if (!lone_lisp_reader_is_expected_character_symbol(lone, next, ')')) {
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
	struct lone_bytes name;
	char character;

	if (reader->status.end_of_input) { return lone_lisp_nil(); }

	/* lexer has already parsed atoms */
	switch (lone_lisp_type_of(token)) {
	case LONE_LISP_TAG_NIL:
	case LONE_LISP_TAG_FALSE:
	case LONE_LISP_TAG_TRUE:
	case LONE_LISP_TAG_INTEGER:
		return token;
	case LONE_LISP_TAG_BYTES:
	case LONE_LISP_TAG_TEXT:
		return token;
	case LONE_LISP_TAG_SYMBOL:

		name = lone_lisp_bytes_of(lone, &token);

		if (name.count > 1) { return token; }
		character = *name.pointer;

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

		__builtin_unreachable();
		break;

	case LONE_LISP_TAG_MODULE:
	case LONE_LISP_TAG_FUNCTION:
	case LONE_LISP_TAG_PRIMITIVE:
	case LONE_LISP_TAG_CONTINUATION:
	case LONE_LISP_TAG_GENERATOR:
	case LONE_LISP_TAG_LIST:
	case LONE_LISP_TAG_VECTOR:
	case LONE_LISP_TAG_TABLE:
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
