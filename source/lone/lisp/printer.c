/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/printer.h>

#include <lone/lisp/value/list.h>
#include <lone/lisp/value/vector.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/pointer.h>

#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/linux.h>

static void lone_lisp_print_integer(int fd, long n)
{
	static char digits[LONE_LISP_DECIMAL_DIGITS_PER_INTEGER + 1]; /* digits + sign */
	char *digit;
	bool is_negative;
	size_t count;

	digit = digits + LONE_LISP_DECIMAL_DIGITS_PER_INTEGER;  /* work backwards */
	count = 0;

	if (n < 0) {
		is_negative = true;
		n *= -1;
	} else {
		is_negative = false;
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

static void lone_lisp_print_pointer(struct lone_lisp *lone, struct lone_lisp_value pointer, int fd)
{
	lone_lisp_print_integer(fd, (intptr_t) lone_lisp_value_to_pointer(pointer));
}

static void lone_lisp_print_bytes(struct lone_lisp *lone, struct lone_lisp_value bytes, int fd)
{
	static unsigned char hexadecimal[] = "0123456789ABCDEF";
	unsigned char *text, *byte, low, high;
	size_t size, count, i;

	count = lone_lisp_value_to_heap_value(bytes)->as.bytes.count;

	if (count == 0) { linux_write(fd, "bytes[]", 7); return; }

	size = 2 + count * 2; /* "0x" + 2 characters per input byte */
	text = lone_allocate_uninitialized(lone->system, size);
	byte = lone_lisp_value_to_heap_value(bytes)->as.bytes.pointer;

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

	lone_deallocate(lone->system, text);
}

static void lone_lisp_print_list(struct lone_lisp *lone, struct lone_lisp_value list, int fd)
{
	struct lone_lisp_value first, rest;

	first = lone_lisp_list_first(list);
	rest = lone_lisp_list_rest(list);

	lone_lisp_print(lone, first, fd);

	if (lone_lisp_is_list(rest)) {
		linux_write(fd, " ", 1);
		lone_lisp_print_list(lone, rest, fd);
	} else if (!lone_lisp_is_nil(rest)) {
		linux_write(fd, " . ", 3);
		lone_lisp_print(lone, rest, fd);
	}
}

static void lone_lisp_print_vector(struct lone_lisp *lone, struct lone_lisp_value vector, int fd)
{
	struct lone_lisp_value element;
	size_t count, i;

	count = lone_lisp_vector_count(vector);

	if (count == 0) { linux_write(fd, "[]", 2); return; }

	linux_write(fd, "[ ", 2);

	LONE_LISP_VECTOR_FOR_EACH(element, vector, i) {
		lone_lisp_print(lone, element, fd);
		linux_write(fd, " ", 1);
	}

	linux_write(fd, "]", 1);
}

static void lone_lisp_print_table(struct lone_lisp *lone, struct lone_lisp_value table, int fd)
{
	struct lone_lisp_table_entry *entries;
	size_t count, i;

	count = lone_lisp_table_count(table);

	if (count == 0) { linux_write(fd, "{}", 2); return; }

	entries = lone_lisp_value_to_heap_value(table)->as.table.entries;

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

	arguments = lone_lisp_value_to_heap_value(function)->as.function.arguments;
	code = lone_lisp_value_to_heap_value(function)->as.function.code;

	linux_write(fd, "(𝛌 ", 6);
	lone_lisp_print(lone, arguments, fd);

	while (!lone_lisp_is_nil(code)) {
		linux_write(fd, "\n  ", 3);
		lone_lisp_print(lone, lone_lisp_list_first(code), fd);
		code = lone_lisp_list_rest(code);
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
	switch (lone_lisp_value_to_type(value)) {
	case LONE_LISP_TYPE_NIL:
		linux_write(fd, "nil", 3);
		return;
	case LONE_LISP_TYPE_INTEGER:
		lone_lisp_print_integer(fd, lone_lisp_value_to_integer(value));
		return;
	case LONE_LISP_TYPE_POINTER:
		lone_lisp_print_pointer(lone, value, fd);
		return;
	case LONE_LISP_TYPE_HEAP_VALUE:
		break;
	}

	switch (lone_lisp_value_to_heap_value(value)->type) {
	case LONE_LISP_TYPE_MODULE:
		lone_lisp_print_hash_notation(lone, "module",
				lone_lisp_value_to_heap_value(value)->as.module.name, fd);
		break;
	case LONE_LISP_TYPE_PRIMITIVE:
		lone_lisp_print_hash_notation(lone, "primitive",
				lone_lisp_value_to_heap_value(value)->as.primitive.name, fd);
		break;
	case LONE_LISP_TYPE_FUNCTION:
		lone_lisp_print_function(lone, value, fd);
		break;
	case LONE_LISP_TYPE_LIST:
		linux_write(fd, "(", 1);
		lone_lisp_print_list(lone, value, fd);
		linux_write(fd, ")", 1);
		break;
	case LONE_LISP_TYPE_VECTOR:
		lone_lisp_print_vector(lone, value, fd);
		break;
	case LONE_LISP_TYPE_TABLE:
		lone_lisp_print_table(lone, value, fd);
		break;
	case LONE_LISP_TYPE_BYTES:
		lone_lisp_print_bytes(lone, value, fd);
		break;
	case LONE_LISP_TYPE_SYMBOL:
		linux_write(fd,
				lone_lisp_value_to_heap_value(value)->as.bytes.pointer,
				lone_lisp_value_to_heap_value(value)->as.bytes.count);
		break;
	case LONE_LISP_TYPE_TEXT:
		linux_write(fd, "\"", 1);
		linux_write(fd,
				lone_lisp_value_to_heap_value(value)->as.bytes.pointer,
				lone_lisp_value_to_heap_value(value)->as.bytes.count);
		linux_write(fd, "\"", 1);
		break;
	}
}
