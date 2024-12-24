/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/value/symbol.h>
#include <lone/lisp/value/bytes.h>
#include <lone/lisp/value/table.h>

#include <lone/memory/functions.h>

static struct lone_lisp_value lone_lisp_symbol_transfer(struct lone_lisp *lone,
		unsigned char *text, size_t length, bool should_deallocate)
{
	struct lone_lisp_value value = lone_lisp_bytes_transfer(lone, text, length, should_deallocate);
	lone_lisp_value_to_heap_value(value)->type = LONE_LISP_TYPE_SYMBOL;
	return value;
}

static struct lone_lisp_value lone_lisp_symbol_transfer_bytes(struct lone_lisp *lone,
		struct lone_bytes bytes, bool should_deallocate)
{
	return lone_lisp_symbol_transfer(lone, bytes.pointer, bytes.count, should_deallocate);
}

static struct lone_lisp_value lone_lisp_symbol_copy(struct lone_lisp *lone,
		unsigned char *text, size_t length)
{
	struct lone_lisp_value value = lone_lisp_bytes_copy(lone, text, length);
	lone_lisp_value_to_heap_value(value)->type = LONE_LISP_TYPE_SYMBOL;
	return value;
}

struct lone_lisp_value lone_lisp_intern(struct lone_lisp *lone,
		unsigned char *bytes, size_t count, bool should_deallocate)
{
	struct lone_lisp_value key, value;

	key = should_deallocate?
		  lone_lisp_bytes_copy(lone, bytes, count)
		: lone_lisp_bytes_transfer(lone, bytes, count, should_deallocate);

	value = lone_lisp_table_get(lone, lone->symbol_table, key);

	if (lone_lisp_is_nil(value)) {
		value = should_deallocate?
			  lone_lisp_symbol_copy(lone, bytes, count)
			: lone_lisp_symbol_transfer(lone, bytes, count, should_deallocate);

		lone_lisp_table_set(lone, lone->symbol_table, key, value);
	}

	return value;
}

struct lone_lisp_value lone_lisp_intern_bytes(struct lone_lisp *lone,
		struct lone_bytes bytes, bool should_deallocate)
{
	return lone_lisp_intern(lone, bytes.pointer, bytes.count, should_deallocate);
}

struct lone_lisp_value lone_lisp_intern_c_string(struct lone_lisp *lone, char *c_string)
{
	return lone_lisp_intern(lone, (unsigned char *) c_string, lone_c_string_length(c_string), false);
}

struct lone_lisp_value lone_lisp_intern_text(struct lone_lisp *lone, struct lone_lisp_value text)
{
	return lone_lisp_intern_bytes(lone, lone_lisp_value_to_heap_value(text)->as.bytes, true);
}
