/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/value/bytes.h>
#include <lone/value/table.h>
#include <lone/memory/functions.h>

#include <lone/struct/lisp.h>
#include <lone/struct/value.h>

struct lone_value *lone_symbol_transfer(struct lone_lisp *lone, unsigned char *text, size_t length, bool should_deallocate)
{
	struct lone_value *value = lone_bytes_transfer(lone, text, length, should_deallocate);
	value->type = LONE_SYMBOL;
	return value;
}

struct lone_value *lone_symbol_transfer_bytes(struct lone_lisp *lone, struct lone_bytes bytes, bool should_deallocate)
{
	return lone_symbol_transfer(lone, bytes.pointer, bytes.count, should_deallocate);
}

struct lone_value *lone_symbol_create(struct lone_lisp *lone, unsigned char *text, size_t length)
{
	struct lone_value *value = lone_bytes_create(lone, text, length);
	value->type = LONE_SYMBOL;
	return value;
}

struct lone_value *lone_intern(struct lone_lisp *lone, unsigned char *bytes, size_t count, bool should_deallocate)
{
	struct lone_value *key, *value;

	key = should_deallocate? lone_symbol_create(lone, bytes, count) : lone_symbol_transfer(lone, bytes, count, should_deallocate);
	value = lone_table_get(lone, lone->symbol_table, key);

	if (lone_is_nil(value)) {
		value = key;
		lone_table_set(lone, lone->symbol_table, key, value);
	}

	return value;
}

struct lone_value *lone_intern_c_string(struct lone_lisp *lone, char *c_string)
{
	return lone_intern(lone, (unsigned char *) c_string, lone_c_string_length(c_string), false);
}
