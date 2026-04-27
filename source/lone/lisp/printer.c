/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/printer.h>

#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/linux.h>

static void lone_lisp_print_integer(int fd, long n)
{
	char digits[LONE_LISP_DECIMAL_DIGITS_PER_INTEGER + 1]; /* digits + sign */
	char *digit;
	bool is_negative;
	unsigned long magnitude;
	size_t count;

	digit = digits + LONE_LISP_DECIMAL_DIGITS_PER_INTEGER;  /* work backwards */
	count = 0;

	if (n < 0) {
		is_negative = true;
		magnitude = -((unsigned long) n);
	} else {
		is_negative = false;
		magnitude = (unsigned long) n;
	}

	do {
		*--digit = '0' + (magnitude % 10);
		magnitude /= 10;
		++count;
	} while (magnitude > 0);

	if (is_negative) {
		*--digit = '-';
		++count;
	}

	linux_write_bytes(fd, LONE_BYTES_VALUE(count, digit));
}

static void lone_lisp_print_escaped_content(struct lone_bytes content, int fd)
{
	static const char hex[] = "0123456789abcdef";
	unsigned char *byte;
	unsigned char hex_buffer[4];
	char *escape;
	size_t i, run_start, escape_length;

	run_start = 0;

	for (i = 0; i < content.count; ++i) {
		byte = &content.pointer[i];

		#define ESCAPE(character, escaped) \
			case character: \
			escape = escaped; \
			escape_length = sizeof(escaped) - 1; \
			break

		switch (*byte) {
		ESCAPE('\\', "\\\\");
		ESCAPE('"',  "\\\"");
		ESCAPE('\n', "\\n");
		ESCAPE('\t', "\\t");
		ESCAPE('\0', "\\0");
		default:
			if (*byte >= 0x20 && *byte <= 0x7E) { continue; }
			hex_buffer[0] = '\\';
			hex_buffer[1] = 'x';
			hex_buffer[2] = hex[(*byte >> 4) & 0xF];
			hex_buffer[3] = hex[*byte & 0xF];
			escape = (char *) hex_buffer;
			escape_length = 4;
			break;
		}

		#undef ESCAPE

		if (i > run_start) {
			linux_write_bytes(fd, LONE_BYTES_VALUE(i - run_start, content.pointer + run_start));
		}

		linux_write_bytes(fd, LONE_BYTES_VALUE(escape_length, escape));
		run_start = i + 1;
	}

	if (content.count > run_start) {
		linux_write_bytes(fd, LONE_BYTES_VALUE(content.count - run_start, content.pointer + run_start));
	}
}

static void lone_lisp_print_bytes(struct lone_lisp *lone, struct lone_lisp_value bytes, int fd)
{
	struct lone_bytes content;

	content = lone_lisp_bytes_of(lone, &bytes);

	linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("b\""));
	lone_lisp_print_escaped_content(content, fd);
	linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("\""));
}

static void lone_lisp_print_list(struct lone_lisp *lone, struct lone_lisp_value list, int fd)
{
	struct lone_lisp_value first, rest;

	first = lone_lisp_list_first(lone, list);
	rest = lone_lisp_list_rest(lone, list);

	lone_lisp_print(lone, first, fd);

	if (lone_lisp_is_nil(rest)) {
		/* proper list termination */
	} else if (lone_lisp_is_list(lone, rest)) {
		linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL(" "));
		lone_lisp_print_list(lone, rest, fd);
	} else {
		linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL(" . "));
		lone_lisp_print(lone, rest, fd);
	}
}

static void lone_lisp_print_vector(struct lone_lisp *lone, struct lone_lisp_value vector, int fd)
{
	struct lone_lisp_value element;
	size_t count, i;

	count = lone_lisp_vector_count(lone, vector);

	if (count == 0) { linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("[]")); return; }

	linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("[ "));

	LONE_LISP_VECTOR_FOR_EACH(lone, element, vector, i) {
		lone_lisp_print(lone, element, fd);
		linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL(" "));
	}

	linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("]"));
}

static void lone_lisp_print_table(struct lone_lisp *lone, struct lone_lisp_value table, int fd)
{
	struct lone_lisp_table *actual;
	size_t i;

	if (lone_lisp_table_count(lone, table) == 0) { linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("{}")); return; }

	actual = &lone_lisp_heap_value_of(lone, table)->as.table;

	linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("{ "));

	for (i = 0; i < actual->used; ++i) {
		if (lone_lisp_is_tombstone(actual->entries[i].key)) { continue; }
		lone_lisp_print(lone, actual->entries[i].key, fd);
		linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL(" "));
		lone_lisp_print(lone, actual->entries[i].value, fd);
		linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL(" "));
	}

	linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("}"));
}

static void lone_lisp_print_function(struct lone_lisp *lone, struct lone_lisp_value function, int fd)
{
	struct lone_lisp_value arguments, code;

	arguments = lone_lisp_heap_value_of(lone, function)->as.function.arguments;
	code = lone_lisp_heap_value_of(lone, function)->as.function.code;

	linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("(𝛌 "));
	lone_lisp_print(lone, arguments, fd);

	while (!lone_lisp_is_nil(code)) {
		linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("\n  "));
		lone_lisp_print(lone, lone_lisp_list_first(lone, code), fd);
		code = lone_lisp_list_rest(lone, code);
	}

	linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL(")"));
}

static void lone_lisp_print_text(struct lone_lisp *lone, struct lone_lisp_value value, int fd)
{
	struct lone_bytes text;

	text = lone_lisp_bytes_of(lone, &value);

	linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("\""));
	lone_lisp_print_escaped_content(text, fd);
	linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("\""));
}

static void lone_lisp_print_hash_notation(struct lone_lisp *lone, char *descriptor, struct lone_lisp_value value, int fd)
{
	linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("#<"));
	linux_write_bytes(fd, LONE_BYTES_VALUE(lone_c_string_length(descriptor), descriptor));
	linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL(" "));
	lone_lisp_print(lone, value, fd);
	linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL(">"));
}

void lone_lisp_print(struct lone_lisp *lone, struct lone_lisp_value value, int fd)
{
	switch (lone_lisp_type_of(value)) {
	case LONE_LISP_TAG_NIL:
		linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("()"));
		return;
	case LONE_LISP_TAG_FALSE:
		linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("false"));
		return;
	case LONE_LISP_TAG_TRUE:
		linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("true"));
		return;
	case LONE_LISP_TAG_INTEGER:
		lone_lisp_print_integer(fd, lone_lisp_integer_of(value));
		return;
	case LONE_LISP_TAG_MODULE:
		lone_lisp_print_hash_notation(lone, "module",
				lone_lisp_heap_value_of(lone, value)->as.module.name, fd);
		break;
	case LONE_LISP_TAG_PRIMITIVE:
		lone_lisp_print_hash_notation(lone, "primitive",
				lone_lisp_heap_value_of(lone, value)->as.primitive.name, fd);
		break;
	case LONE_LISP_TAG_FUNCTION:
		lone_lisp_print_function(lone, value, fd);
		break;
	case LONE_LISP_TAG_CONTINUATION:
		lone_lisp_print_hash_notation(lone, "continuation",
				lone_lisp_integer_create(lone_lisp_heap_value_of(lone, value)->as.continuation.frame_count), fd);
		break;
	case LONE_LISP_TAG_GENERATOR:
		lone_lisp_print_hash_notation(
			lone,
			"generator",
			lone_lisp_heap_value_of(lone, value)->as.generator.function,
			fd
		);
		break;
	case LONE_LISP_TAG_LIST:
		linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL("("));
		lone_lisp_print_list(lone, value, fd);
		linux_write_bytes(fd, LONE_BYTES_VALUE_FROM_LITERAL(")"));
		break;
	case LONE_LISP_TAG_VECTOR:
		lone_lisp_print_vector(lone, value, fd);
		break;
	case LONE_LISP_TAG_TABLE:
		lone_lisp_print_table(lone, value, fd);
		break;
	case LONE_LISP_TAG_BYTES:
		lone_lisp_print_bytes(lone, value, fd);
		break;
	case LONE_LISP_TAG_SYMBOL: {
		struct lone_bytes name = lone_lisp_bytes_of(lone, &value);
		linux_write_bytes(fd, name);
		break;
	}
	case LONE_LISP_TAG_TEXT:
		lone_lisp_print_text(lone, value, fd);
		break;
	}
}
