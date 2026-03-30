/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>
#include <lone/lisp/heap.h>
#include <lone/lisp/hash.h>

#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

static struct lone_lisp_value lone_lisp_symbol_transfer(struct lone_lisp *lone,
		unsigned char *text, size_t length, bool should_deallocate)
{
	struct lone_lisp_heap_value *actual = lone_lisp_heap_allocate_value(lone);
	actual->type = LONE_LISP_TYPE_SYMBOL;
	actual->as.symbol.name.count = length;
	actual->as.symbol.name.pointer = text;
	actual->should_deallocate_bytes = should_deallocate;
	actual->as.symbol.hash = lone_lisp_hash_as_symbol(lone, actual->as.symbol.name);
	return lone_lisp_value_from_heap_value(lone, actual);
}

static struct lone_lisp_value lone_lisp_symbol_transfer_bytes(struct lone_lisp *lone,
		struct lone_bytes bytes, bool should_deallocate)
{
	return lone_lisp_symbol_transfer(lone, bytes.pointer, bytes.count, should_deallocate);
}

static struct lone_lisp_value lone_lisp_symbol_copy(struct lone_lisp *lone,
		unsigned char *text, size_t length)
{
	unsigned char *copy = lone_memory_allocate(lone->system, length + 1, 1, 1, LONE_MEMORY_ALLOCATION_FLAGS_NONE);
	lone_memory_move(text, copy, length);
	copy[length] = '\0';
	return lone_lisp_symbol_transfer(lone, copy, length, true);
}

struct lone_lisp_value lone_lisp_intern(struct lone_lisp *lone,
		unsigned char *bytes, size_t count, bool should_deallocate)
{
	struct lone_bytes name = { count, bytes };
	struct lone_lisp_value value;

	value = lone_lisp_table_get_by_symbol(lone, lone->symbol_table, name);

	if (lone_lisp_is_nil(value)) {

		value =
			  should_deallocate
			? lone_lisp_symbol_copy(lone, bytes, count)
			: lone_lisp_symbol_transfer(lone, bytes, count, should_deallocate);

		lone_lisp_table_set(lone, lone->symbol_table, value, value);
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
	return lone_lisp_intern_bytes(lone, lone_lisp_heap_value_of(lone, text)->as.bytes, true);
}
