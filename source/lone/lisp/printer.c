/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/lisp/printer.h>
#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/value/list.h>
#include <lone/value/vector.h>
#include <lone/value/table.h>
#include <lone/value/pointer.h>

#include <lone/linux.h>

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

static void lone_print_pointer(struct lone_lisp *lone, struct lone_value pointer, int fd)
{
	if (pointer.pointer_type == LONE_TO_UNKNOWN) {
		lone_print_integer(fd, (intptr_t) pointer.as.pointer.to_void);
	} else {
		lone_print(lone, lone_pointer_dereference(pointer), fd);
	}
}

static void lone_print_bytes(struct lone_lisp *lone, struct lone_value bytes, int fd)
{
	static unsigned char hexadecimal[] = "0123456789ABCDEF";
	unsigned char *text, *byte, low, high;
	struct lone_heap_value *actual;
	size_t size, count, i;

	actual = bytes.as.heap_value;
	count = actual->as.bytes.count;

	if (count == 0) { linux_write(fd, "bytes[]", 7); return; }

	size = 2 + count * 2; /* "0x" + 2 characters per input byte */
	text = lone_allocate_uninitialized(lone, size);
	byte = actual->as.bytes.pointer;

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

	lone_deallocate(lone, text);
}

static void lone_print_list(struct lone_lisp *lone, struct lone_value list, int fd)
{
	struct lone_value first, rest;

	first = lone_list_first(list);
	rest = lone_list_rest(list);

	lone_print(lone, first, fd);

	if (lone_is_list(rest)) {
		linux_write(fd, " ", 1);
		lone_print_list(lone, rest, fd);
	} else if (!lone_is_nil(rest)) {
		linux_write(fd, " . ", 3);
		lone_print(lone, rest, fd);
	}
}

static void lone_print_vector(struct lone_lisp *lone, struct lone_value vector, int fd)
{
	struct lone_heap_value *actual;
	struct lone_value *values;
	size_t count, i;

	actual = vector.as.heap_value;
	count = lone_vector_count(vector);

	if (count == 0) { linux_write(fd, "[]", 2); return; }

	values = actual->as.vector.values;

	linux_write(fd, "[ ", 2);

	for (i = 0; i < count; ++i) {
		lone_print(lone, values[i], fd);
		linux_write(fd, " ", 1);
	}

	linux_write(fd, "]", 1);
}

static void lone_print_table(struct lone_lisp *lone, struct lone_value table, int fd)
{
	struct lone_heap_value *actual;
	struct lone_table_entry *entries;
	size_t count, i;

	actual = table.as.heap_value;
	count = actual->as.table.count;

	if (count == 0) { linux_write(fd, "{}", 2); return; }

	entries = actual->as.table.entries;

	linux_write(fd, "{ ", 2);

	for (i = 0; i < count; ++i) {
		lone_print(lone, entries[i].key, fd);
		linux_write(fd, " ", 1);
		lone_print(lone, entries[i].value, fd);
		linux_write(fd, " ", 1);
	}

	linux_write(fd, "}", 1);
}

static void lone_print_function(struct lone_lisp *lone, struct lone_value function, int fd)
{
	struct lone_heap_value *actual;
	struct lone_value arguments, code;

	actual = function.as.heap_value;
	arguments = actual->as.function.arguments;
	code = actual->as.function.code;

	linux_write(fd, "(𝛌 ", 6);
	lone_print(lone, arguments, fd);

	while (!lone_is_nil(code)) {
		linux_write(fd, "\n  ", 3);
		lone_print(lone, lone_list_first(code), fd);
		code = lone_list_rest(code);
	}

	linux_write(fd, ")", 1);
}

static void lone_print_hash_notation(struct lone_lisp *lone, char *descriptor, struct lone_value value, int fd)
{
	linux_write(fd, "#<", 2);
	linux_write(fd, descriptor, lone_c_string_length(descriptor));
	linux_write(fd, " ", 1);
	lone_print(lone, value, fd);
	linux_write(fd, ">", 1);
}

void lone_print(struct lone_lisp *lone, struct lone_value value, int fd)
{
	struct lone_heap_value *actual;

	switch (value.type) {
	case LONE_NIL:
		linux_write(fd, "nil", 3);
		return;
	case LONE_INTEGER:
		lone_print_integer(fd, value.as.signed_integer);
		return;
	case LONE_POINTER:
		lone_print_pointer(lone, value, fd);
		return;
	case LONE_HEAP_VALUE:
		break;
	}

	actual = value.as.heap_value;

	switch (actual->type) {
	case LONE_MODULE:
		lone_print_hash_notation(lone, "module", actual->as.module.name, fd);
		break;
	case LONE_PRIMITIVE:
		lone_print_hash_notation(lone, "primitive", actual->as.primitive.name, fd);
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
		linux_write(fd, actual->as.bytes.pointer, actual->as.bytes.count);
		break;
	case LONE_TEXT:
		linux_write(fd, "\"", 1);
		linux_write(fd, actual->as.bytes.pointer, actual->as.bytes.count);
		linux_write(fd, "\"", 1);
		break;
	}
}
