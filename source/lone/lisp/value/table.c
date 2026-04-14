/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>
#include <lone/lisp/heap.h>
#include <lone/lisp/hash.h>

#include <lone/memory/allocator.h>
#include <lone/memory/array.h>
#include <lone/memory/functions.h>

#include <lone/utilities.h>

struct lone_lisp_value lone_lisp_table_create(struct lone_lisp *lone,
		size_t capacity, struct lone_lisp_value prototype)
{
	struct lone_lisp_heap_value *heap_value = lone_lisp_heap_allocate_value(lone);
	struct lone_lisp_table *actual = &heap_value->as.table;

	capacity = lone_next_power_of_2(capacity);

	actual->prototype = prototype;
	actual->count = 0;
	actual->capacity = capacity;
	actual->used = 0;

	actual->indexes = lone_memory_array(
		lone->system,
		0,
		0,
		actual->capacity,
		sizeof(*actual->indexes),
		alignof(*actual->indexes)
	);

	lone_memory_one(actual->indexes, lone_memory_array_size_in_bytes(capacity, sizeof(*actual->indexes)));

	actual->entries = lone_memory_array(
		lone->system,
		0,
		0,
		actual->capacity,
		sizeof(*actual->entries),
		alignof(*actual->entries)
	);

	return lone_lisp_value_from_heap_value(lone, heap_value, LONE_LISP_TAG_TABLE);
}

size_t lone_lisp_table_count(struct lone_lisp *lone, struct lone_lisp_value table)
{
	return lone_lisp_heap_value_of(lone, table)->as.table.count;
}

static bool lone_lisp_table_needs_resize(struct lone_lisp *lone, struct lone_lisp_value table, unsigned char added)
{
	struct lone_lisp_table *actual;
	size_t count, capacity;

	actual = &lone_lisp_heap_value_of(lone, table)->as.table;
	count = actual->count + added;
	capacity = actual->capacity;

	return (count * LONE_LISP_TABLE_LOAD_FACTOR_DENOMINATOR) > (capacity * LONE_LISP_TABLE_LOAD_FACTOR_NUMERATOR);
}

static bool lone_lisp_table_is_empty(size_t *indexes, size_t index)
{
	return indexes[index] == LONE_LISP_TABLE_INDEX_EMPTY;
}

static bool lone_lisp_table_is_used(size_t *indexes, size_t index)
{
	return !lone_lisp_table_is_empty(indexes, index);
}

static unsigned long lone_lisp_table_wrap_around(size_t index, size_t capacity)
{
	return index & (capacity - 1);
}

static unsigned long lone_lisp_table_compute_hash_for(struct lone_lisp *lone,
		struct lone_lisp_value key, size_t capacity)
{
	return lone_lisp_table_wrap_around(lone_lisp_hash(lone, key), capacity);
}

static bool lone_lisp_table_key_matches(struct lone_lisp *lone,
		struct lone_lisp_value stored, struct lone_lisp_value key)
{
	/* Word comparison suffices for identity-comparable types:
	 *
	 * 	interned symbols: same name, same heap index
	 * 	integers: value encoded in data bits
	 * 	singletons: nil, true, false
	 *
	 * Fall through to structural comparison only for types
	 * where distinct heap objects can be equal:
	 *
	 * 	lists
	 * 	texts
	 * 	bytes
	 *
	 */
	if (stored.tagged == key.tagged) { return true; }

	switch (lone_lisp_type_of(key)) {
	case LONE_LISP_TAG_LIST:
	case LONE_LISP_TAG_TEXT:
	case LONE_LISP_TAG_BYTES:
		return lone_lisp_is_equal(lone, stored, key);
	default:
		return false;
	}
}

static size_t lone_lisp_table_entry_find_index_for(struct lone_lisp *lone, struct lone_lisp_value key,
		size_t *indexes, struct lone_lisp_table_entry *entries, size_t capacity)
{
	size_t i = lone_lisp_table_compute_hash_for(lone, key, capacity);

	while (lone_lisp_table_is_used(indexes, i) && !lone_lisp_table_key_matches(lone, entries[indexes[i]].key, key)) {
		i = lone_lisp_table_wrap_around(i + 1, capacity);
	}

	return i;
}

static bool lone_lisp_table_bytes_is_equal(struct lone_lisp *lone,
		struct lone_lisp_value x_value,
		struct lone_bytes y_bytes, enum lone_lisp_tag y_tag, unsigned char y_hash_bits)
{
	struct lone_bytes x_bytes;
	enum lone_lisp_tag x_tag;
	unsigned char x_hash_bits;

	x_tag = lone_lisp_type_of(x_value);
	if (x_tag != y_tag) { return false; }

	/* For heap symbols, compare 8 hash bits stored in metadata at bits 8-15
	 * against the search hash bits before accessing the heap.
	 * Rejects 255/256 of non-matching entries without a heap dereference.
	 */
	if (x_tag == LONE_LISP_TAG_SYMBOL && !lone_lisp_is_inline_value(x_value)) {
		x_hash_bits = (x_value.tagged >> LONE_LISP_METADATA_SHIFT) & 0xFF;
		if (x_hash_bits != y_hash_bits) { return false; }
	}

	x_bytes = lone_lisp_bytes_of(lone, &x_value);

	return lone_bytes_is_equal(x_bytes, y_bytes);
}

static size_t lone_lisp_table_entry_find_index_by(struct lone_lisp *lone,
		unsigned long hash, struct lone_bytes bytes, enum lone_lisp_tag type,
		size_t *indexes, struct lone_lisp_table_entry *entries, size_t capacity)
{
	unsigned char hash_bits = (unsigned char) hash;
	size_t i = lone_lisp_table_wrap_around(hash, capacity);

	while (lone_lisp_table_is_used(indexes, i)
	       && !lone_lisp_table_bytes_is_equal(lone, entries[indexes[i]].key, bytes, type, hash_bits)) {
		i = lone_lisp_table_wrap_around(i + 1, capacity);
	}

	return i;
}

static void lone_lisp_table_resize(struct lone_lisp *lone, struct lone_lisp_value table, size_t new_capacity)
{
	struct lone_lisp_table *actual;
	struct lone_lisp_table_entry *old_entries, *new_entries;
	size_t *old_indexes, *new_indexes;
	size_t old_capacity, old_used;
	size_t i, j, index;

	actual = &lone_lisp_heap_value_of(lone, table)->as.table;

	old_capacity = actual->capacity;
	old_used     = actual->used;
	old_entries  = actual->entries;
	old_indexes  = actual->indexes;

	new_indexes = lone_memory_array(
		lone->system,
		0,
		0,
		new_capacity,
		sizeof(*new_indexes),
		alignof(*new_indexes)
	);

	lone_memory_one(new_indexes, lone_memory_array_size_in_bytes(new_capacity, sizeof(*new_indexes)));

	new_entries = lone_memory_array(
		lone->system,
		0,
		0,
		new_capacity,
		sizeof(*new_entries),
		alignof(*new_entries)
	);

	for (i = 0, j = 0; i < old_used; ++i) {
		if (lone_lisp_is_tombstone(old_entries[i].key)) { continue; }

		new_entries[j].key = old_entries[i].key;
		new_entries[j].value = old_entries[i].value;

		index = lone_lisp_table_entry_find_index_for(lone,
				new_entries[j].key, new_indexes, new_entries, new_capacity);
		new_indexes[index] = j;
		++j;
	}

	lone_memory_deallocate(lone->system, old_indexes, old_capacity, sizeof(*old_indexes), alignof(*old_indexes));
	lone_memory_deallocate(lone->system, old_entries, old_capacity, sizeof(*old_entries), alignof(*old_entries));

	actual->indexes = new_indexes;
	actual->entries = new_entries;
	actual->capacity = new_capacity;
	actual->used = j;
}

void lone_lisp_table_set(struct lone_lisp *lone, struct lone_lisp_value table,
		struct lone_lisp_value key, struct lone_lisp_value value)
{
	struct lone_lisp_table *actual;
	size_t i, new_capacity;
	bool resize;

	actual = &lone_lisp_heap_value_of(lone, table)->as.table;
	i = lone_lisp_table_entry_find_index_for(lone, key,
			actual->indexes, actual->entries, actual->capacity);

	if (lone_lisp_table_is_used(actual->indexes, i)) {
		actual->entries[actual->indexes[i]].value = value;
	} else {
		resize = lone_lisp_table_needs_resize(lone, table, 1);
		if (actual->used >= actual->capacity || resize) {
			new_capacity = resize? actual->capacity * LONE_LISP_TABLE_GROWTH_FACTOR : actual->capacity;
			lone_lisp_table_resize(lone, table, new_capacity);
			actual = &lone_lisp_heap_value_of(lone, table)->as.table;

			i = lone_lisp_table_entry_find_index_for(
				lone,
				key,
				actual->indexes,
				actual->entries,
				actual->capacity
			);
		}

		actual->indexes[i] = actual->used;
		actual->entries[actual->used].key = key;
		actual->entries[actual->used].value = value;
		++actual->used;
		++actual->count;
	}
}

struct lone_lisp_value lone_lisp_table_get(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_lisp_value key)
{
	struct lone_lisp_table *actual;
	struct lone_lisp_table_entry *entries;
	size_t *indexes;
	size_t capacity, i;

	actual = &lone_lisp_heap_value_of(lone, table)->as.table;
	indexes = actual->indexes;
	entries = actual->entries;
	capacity = actual->capacity;

	i = lone_lisp_table_entry_find_index_for(lone, key, indexes, entries, capacity);

	if (lone_lisp_table_is_used(indexes, i)) {
		return entries[indexes[i]].value;
	} else if (!lone_lisp_is_nil(actual->prototype)) {
		return lone_lisp_table_get(lone, actual->prototype, key);
	} else {
		return lone_lisp_nil();
	}
}

static struct lone_lisp_value lone_lisp_table_get_by(struct lone_lisp *lone, struct lone_lisp_value table,
		unsigned long hash, struct lone_bytes bytes, enum lone_lisp_tag type)
{
	struct lone_lisp_table *actual;
	struct lone_lisp_table_entry *entries;
	size_t *indexes;
	size_t capacity, i;

	actual = &lone_lisp_heap_value_of(lone, table)->as.table;
	indexes = actual->indexes;
	entries = actual->entries;
	capacity = actual->capacity;

	i = lone_lisp_table_entry_find_index_by(lone, hash, bytes, type, indexes, entries, capacity);

	if (lone_lisp_table_is_used(indexes, i)) {
		return entries[indexes[i]].value;
	} else if (!lone_lisp_is_nil(actual->prototype)) {
		return lone_lisp_table_get_by(lone, actual->prototype, hash, bytes, type);
	} else {
		return lone_lisp_nil();
	}
}

struct lone_lisp_value lone_lisp_table_get_by_symbol(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_bytes bytes)
{
	return lone_lisp_table_get_by(lone, table, lone_lisp_hash_as_symbol(lone, bytes), bytes, LONE_LISP_TAG_SYMBOL);
}

struct lone_lisp_value lone_lisp_table_get_by_text(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_bytes bytes)
{
	return lone_lisp_table_get_by(lone, table, lone_lisp_hash_as_text(lone, bytes), bytes, LONE_LISP_TAG_TEXT);
}

struct lone_lisp_value lone_lisp_table_get_by_bytes(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_bytes bytes)
{
	return lone_lisp_table_get_by(lone, table, lone_lisp_hash_as_bytes(lone, bytes), bytes, LONE_LISP_TAG_BYTES);
}

void lone_lisp_table_delete(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_lisp_value key)
{
	struct lone_lisp_table *actual;
	struct lone_lisp_table_entry *entries;
	size_t *indexes;
	size_t capacity;
	size_t i, j, k, l;

	actual = &lone_lisp_heap_value_of(lone, table)->as.table;
	indexes = actual->indexes;
	entries = actual->entries;
	capacity = actual->capacity;

	i = lone_lisp_table_entry_find_index_for(lone, key, indexes, entries, capacity);

	if (lone_lisp_table_is_empty(indexes, i)) { return; }

	l = indexes[i];

	j = i;
	while (1) {
		j = lone_lisp_table_wrap_around(j + 1, capacity);
		if (lone_lisp_table_is_empty(indexes, j)) { break; }
		k = lone_lisp_table_compute_hash_for(lone, entries[indexes[j]].key, capacity);
		if ((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j))) {
			indexes[i] = indexes[j];
			i = j;
		}
	}

	indexes[i] = LONE_LISP_TABLE_INDEX_EMPTY;

	entries[l].key = lone_lisp_tombstone();
	entries[l].value = lone_lisp_tombstone();

	--actual->count;
}

struct lone_lisp_table_entry *lone_lisp_table_next_entry(struct lone_lisp *lone,
		struct lone_lisp_value table, size_t *i)
{
	struct lone_lisp_table *actual = &lone_lisp_heap_value_of(lone, table)->as.table;

	while (*i < actual->used) {
		if (!lone_lisp_is_tombstone(actual->entries[*i].key)) {
			return &actual->entries[*i];
		}
		++(*i);
	}

	return 0;
}
