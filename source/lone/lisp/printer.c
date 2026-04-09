/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/printer.h>

#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/linux.h>

static void lone_lisp_print_integer(int fd, long n)
{
	static char digits[LONE_LISP_DECIMAL_DIGITS_PER_INTEGER + 1]; /* digits + sign */
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

	linux_write(fd, digit, count);
}

static void lone_lisp_print_bytes(struct lone_lisp *lone, struct lone_lisp_value bytes, int fd)
{
	static unsigned char hexadecimal[] = "0123456789ABCDEF";
	unsigned char *text, *byte, low, high;
	size_t size, count, i;

	count = lone_lisp_heap_value_of(lone, bytes)->as.bytes.count;

	if (count == 0) { linux_write(fd, "bytes[]", 7); return; }

	size = 2 + count * 2; /* "0x" + 2 characters per input byte */
	text = lone_memory_allocate(lone->system, size, 1, 1, LONE_MEMORY_ALLOCATION_FLAGS_NONE);
	byte = lone_lisp_heap_value_of(lone, bytes)->as.bytes.pointer;

	text[0] = '0';
	text[1] = 'x';

	for (i = 0; i < count; ++i) {
		low  = (byte[i] & 0x0F) >> 0;
		high = (byte[i] & 0xF0) >> 4;
		text[2 + (2 * i + 0)] = hexadecimal[high];
		text[2 + (2 * i + 1)] = hexadecimal[low];
	}

	linux_write(fd, "bytes[", 6);
	linux_write(fd, text, size);
	linux_write(fd, "]", 1);

	lone_memory_deallocate(lone->system, text, size, 1, 1);
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
		linux_write(fd, " ", 1);
		lone_lisp_print_list(lone, rest, fd);
	} else {
		linux_write(fd, " . ", 3);
		lone_lisp_print(lone, rest, fd);
	}
}

static void lone_lisp_print_vector(struct lone_lisp *lone, struct lone_lisp_value vector, int fd)
{
	struct lone_lisp_value element;
	size_t count, i;

	count = lone_lisp_vector_count(lone, vector);

	if (count == 0) { linux_write(fd, "[]", 2); return; }

	linux_write(fd, "[ ", 2);

	LONE_LISP_VECTOR_FOR_EACH(lone, element, vector, i) {
		lone_lisp_print(lone, element, fd);
		linux_write(fd, " ", 1);
	}

	linux_write(fd, "]", 1);
}

static void lone_lisp_print_table(struct lone_lisp *lone, struct lone_lisp_value table, int fd)
{
	struct lone_lisp_table_entry *entries;
	size_t count, i;

	count = lone_lisp_table_count(lone, table);

	if (count == 0) { linux_write(fd, "{}", 2); return; }

	entries = lone_lisp_heap_value_of(lone, table)->as.table.entries;

	linux_write(fd, "{ ", 2);

	for (i = 0; i < count; ++i) {
		lone_lisp_print(lone, entries[i].key, fd);
		linux_write(fd, " ", 1);
		lone_lisp_print(lone, entries[i].value, fd);
		linux_write(fd, " ", 1);
	}

	linux_write(fd, "}", 1);
}

static void lone_lisp_print_function(struct lone_lisp *lone, struct lone_lisp_value function, int fd)
{
	struct lone_lisp_value arguments, code;

	arguments = lone_lisp_heap_value_of(lone, function)->as.function.arguments;
	code = lone_lisp_heap_value_of(lone, function)->as.function.code;

	linux_write(fd, "(𝛌 ", 6);
	lone_lisp_print(lone, arguments, fd);

	while (!lone_lisp_is_nil(code)) {
		linux_write(fd, "\n  ", 3);
		lone_lisp_print(lone, lone_lisp_list_first(lone, code), fd);
		code = lone_lisp_list_rest(lone, code);
	}

	linux_write(fd, ")", 1);
}

static void lone_lisp_print_hash_notation(struct lone_lisp *lone, char *descriptor, struct lone_lisp_value value, int fd)
{
	linux_write(fd, "#<", 2);
	linux_write(fd, descriptor, lone_c_string_length(descriptor));
	linux_write(fd, " ", 1);
	lone_lisp_print(lone, value, fd);
	linux_write(fd, ">", 1);
}

void lone_lisp_print(struct lone_lisp *lone, struct lone_lisp_value value, int fd)
{
	switch (lone_lisp_type_of(value)) {
	case LONE_LISP_TAG_NIL:
		linux_write(fd, "nil", 3);
		return;
	case LONE_LISP_TAG_FALSE:
		linux_write(fd, "false", 5);
		return;
	case LONE_LISP_TAG_TRUE:
		linux_write(fd, "true", 4);
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
		linux_write(fd, "(", 1);
		lone_lisp_print_list(lone, value, fd);
		linux_write(fd, ")", 1);
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
		struct lone_bytes name = lone_lisp_symbol_name(lone, &value);
		linux_write(fd, name.pointer, name.count);
		break;
	}
	case LONE_LISP_TAG_TEXT: {
		struct lone_bytes text;
		if (lone_lisp_is_inline_text(value)) {
			text = lone_lisp_inline_value_bytes(&value);
		} else {
			text = lone_lisp_heap_value_of(lone, value)->as.bytes;
		}
		linux_write(fd, "\"", 1);
		linux_write(fd, text.pointer, text.count);
		linux_write(fd, "\"", 1);
		break;
	}
	}
}
