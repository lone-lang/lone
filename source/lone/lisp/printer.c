#include <lone/lisp/printer.h>
#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/value/list.h>
#include <lone/value/vector.h>
#include <lone/value/table.h>
#include <lone/value/pointer.h>

#include <lone/linux.h>

#include <lone/struct/value.h>

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

void lone_print(struct lone_lisp *lone, struct lone_value *value, int fd)
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
