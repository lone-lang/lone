/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/value/symbol.h>
#include <lone/value/bytes.h>
#include <lone/value/table.h>
#include <lone/memory/functions.h>

static struct lone_value lone_symbol_transfer(struct lone_lisp *lone, unsigned char *text, size_t length, bool should_deallocate)
{
	struct lone_value value = lone_bytes_transfer(lone, text, length, should_deallocate);
	value.as.heap_value->type = LONE_TYPE_SYMBOL;
	return value;
}

static struct lone_value lone_symbol_transfer_bytes(struct lone_lisp *lone, struct lone_bytes bytes, bool should_deallocate)
{
	return lone_symbol_transfer(lone, bytes.pointer, bytes.count, should_deallocate);
}

static struct lone_value lone_symbol_copy(struct lone_lisp *lone, unsigned char *text, size_t length)
{
	struct lone_value value = lone_bytes_copy(lone, text, length);
	value.as.heap_value->type = LONE_TYPE_SYMBOL;
	return value;
}

struct lone_value lone_intern(struct lone_lisp *lone, unsigned char *bytes, size_t count, bool should_deallocate)
{
	struct lone_value key, value;

	key = should_deallocate? lone_bytes_copy(lone, bytes, count) : lone_bytes_transfer(lone, bytes, count, should_deallocate);
	value = lone_table_get(lone, lone->symbol_table, key);

	if (lone_is_nil(value)) {
		value = should_deallocate? lone_symbol_copy(lone, bytes, count) : lone_symbol_transfer(lone, bytes, count, should_deallocate);
		lone_table_set(lone, lone->symbol_table, key, value);
	}

	return value;
}

struct lone_value lone_intern_bytes(struct lone_lisp *lone, struct lone_bytes bytes, bool should_deallocate)
{
	return lone_intern(lone, bytes.pointer, bytes.count, should_deallocate);
}

struct lone_value lone_intern_c_string(struct lone_lisp *lone, char *c_string)
{
	return lone_intern(lone, (unsigned char *) c_string, lone_c_string_length(c_string), false);
}

struct lone_value lone_intern_text(struct lone_lisp *lone, struct lone_value text)
{
	return lone_intern_bytes(lone, text.as.heap_value->as.bytes, true);
}
