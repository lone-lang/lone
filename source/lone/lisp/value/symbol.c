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
	struct lone_lisp_value value;
	lone_hash hash;

	actual->as.symbol.name.count = length;
	actual->as.symbol.name.pointer = text;
	actual->should_deallocate_bytes = should_deallocate;
	value = lone_lisp_value_from_heap_value(lone, actual, LONE_LISP_TAG_SYMBOL);
	hash = lone_lisp_value_compute_and_store_hash(lone, value);

	/* Store 8 hash bits in the metadata field at bits 8-15.
	 * These bits enable fast rejection during table lookups
	 * using raw bytes without accessing the heap.
	 * Useful for symbol interning, imports.
	 *
	 * 255/256 of non-matching entries are rejected
	 * by comparing the metadata byte against the
	 * search hash byte. The unlikely matches are
	 * compared normally via structural equality.
	 *
	 */
	value.tagged |= (long) ((hash & 0xFF) << LONE_LISP_METADATA_SHIFT);

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

	if (count <= LONE_LISP_INLINE_MAX_LENGTH) {
		return lone_lisp_inline_symbol_create(bytes, count);
	}

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
	struct lone_bytes bytes = lone_lisp_bytes_of(lone, &text);
	return lone_lisp_intern(lone, bytes.pointer, bytes.count,
			!lone_lisp_is_inline_text(text));
}
