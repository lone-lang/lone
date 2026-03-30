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

	heap_value->type = LONE_LISP_TYPE_TABLE;
	actual->prototype = prototype;
	actual->count = 0;
	actual->capacity = capacity;

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

	return lone_lisp_value_from_heap_value(lone, heap_value);
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

static size_t lone_lisp_table_entry_find_index_for(struct lone_lisp *lone, struct lone_lisp_value key,
		size_t *indexes, struct lone_lisp_table_entry *entries, size_t capacity)
{
	size_t i = lone_lisp_table_compute_hash_for(lone, key, capacity);

	while (lone_lisp_table_is_used(indexes, i) && !lone_lisp_is_equal(lone, entries[indexes[i]].key, key)) {
		i = lone_lisp_table_wrap_around(i + 1, capacity);
	}

	return i;
}

static bool lone_lisp_table_bytes_is_equal(struct lone_lisp *lone,
		struct lone_lisp_value x_value, struct lone_bytes y_bytes, enum lone_lisp_heap_value_type y_heap_value_type)
{
	struct lone_lisp_heap_value *x_heap_value;
	struct lone_bytes x_bytes;

	switch (lone_lisp_type_of(x_value)) {
	case LONE_LISP_TYPE_HEAP_VALUE:
		x_heap_value = lone_lisp_heap_value_of(lone, x_value);
		break;
	default:
		return false;
	}

	if (x_heap_value->type != y_heap_value_type) { return false; }

	switch (x_heap_value->type) {
	case LONE_LISP_TYPE_SYMBOL:
		x_bytes = x_heap_value->as.symbol.name;
		break;
	case LONE_LISP_TYPE_TEXT:
	case LONE_LISP_TYPE_BYTES:
		x_bytes = x_heap_value->as.bytes;
		break;
	default:
		return false;
	}

	return lone_bytes_is_equal(x_bytes, y_bytes);
}

static size_t lone_lisp_table_entry_find_index_by(struct lone_lisp *lone,
		unsigned long hash, struct lone_bytes bytes, enum lone_lisp_heap_value_type type,
		size_t *indexes, struct lone_lisp_table_entry *entries, size_t capacity)
{
	size_t i = lone_lisp_table_wrap_around(hash, capacity);

	while (lone_lisp_table_is_used(indexes, i)
	       && !lone_lisp_table_bytes_is_equal(lone, entries[indexes[i]].key, bytes, type)) {
		i = lone_lisp_table_wrap_around(i + 1, capacity);
	}

	return i;
}

static bool lone_lisp_table_entry_set(struct lone_lisp *lone,
		size_t *indexes, struct lone_lisp_table_entry *entries,
		size_t capacity, size_t index_if_new_entry,
		struct lone_lisp_value key, struct lone_lisp_value value)
{
	size_t i = lone_lisp_table_entry_find_index_for(lone, key, indexes, entries, capacity);

	if (lone_lisp_table_is_used(indexes, i)) {
		entries[indexes[i]].value = value;
		return false;
	} else {
		indexes[i] = index_if_new_entry;
		entries[index_if_new_entry].key = key;
		entries[index_if_new_entry].value = value;
		return true;
	}
}

static void lone_lisp_table_resize(struct lone_lisp *lone, struct lone_lisp_value table, size_t new_capacity)
{
	struct lone_lisp_table *actual;
	struct lone_lisp_table_entry *old_entries, *new_entries;
	size_t *old_indexes, *new_indexes;
	size_t old_capacity;
	size_t i;

	actual = &lone_lisp_heap_value_of(lone, table)->as.table;

	old_capacity = actual->capacity;
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
		old_entries,
		old_capacity,
		new_capacity,
		sizeof(*new_entries),
		alignof(*new_entries)
	);

	for (i = 0; i < old_capacity; ++i) {
		if (lone_lisp_table_is_used(old_indexes, i)) {
			lone_lisp_table_entry_set(
				lone,
				new_indexes,
				new_entries,
				new_capacity,
				old_indexes[i],
				new_entries[old_indexes[i]].key,
				new_entries[old_indexes[i]].value
			);
		}
	}

	lone_memory_deallocate(lone->system, old_indexes, old_capacity, sizeof(*old_indexes), alignof(*old_indexes));

	actual->indexes = new_indexes;
	actual->entries = new_entries;
	actual->capacity = new_capacity;
}

void lone_lisp_table_set(struct lone_lisp *lone, struct lone_lisp_value table,
		struct lone_lisp_value key, struct lone_lisp_value value)
{
	struct lone_lisp_table *actual;
	bool is_new_table_entry;

	actual = &lone_lisp_heap_value_of(lone, table)->as.table;

	if (lone_lisp_table_needs_resize(lone, table, 1)) {
		lone_lisp_table_resize(lone, table, actual->capacity * LONE_LISP_TABLE_GROWTH_FACTOR);
	}

	is_new_table_entry = lone_lisp_table_entry_set(
		lone,
		actual->indexes,
		actual->entries,
		actual->capacity,
		actual->count,
		key,
		value
	);

	if (is_new_table_entry) {
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
		unsigned long hash, struct lone_bytes bytes, enum lone_lisp_heap_value_type type)
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
	return lone_lisp_table_get_by(lone, table, lone_lisp_hash_as_symbol(lone, bytes), bytes, LONE_LISP_TYPE_SYMBOL);
}

struct lone_lisp_value lone_lisp_table_get_by_text(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_bytes bytes)
{
	return lone_lisp_table_get_by(lone, table, lone_lisp_hash_as_text(lone, bytes), bytes, LONE_LISP_TYPE_TEXT);
}

struct lone_lisp_value lone_lisp_table_get_by_bytes(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_bytes bytes)
{
	return lone_lisp_table_get_by(lone, table, lone_lisp_hash_as_bytes(lone, bytes), bytes, LONE_LISP_TYPE_BYTES);
}

void lone_lisp_table_delete(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_lisp_value key)
{
	struct lone_lisp_table *actual;
	struct lone_lisp_table_entry *entries;
	size_t *indexes;
	size_t capacity, count;
	size_t i, j, k, l;

	actual = &lone_lisp_heap_value_of(lone, table)->as.table;
	indexes = actual->indexes;
	entries = actual->entries;
	capacity = actual->capacity;
	count = actual->count;

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

	for (i = 0; i < capacity; ++i) {

		if (i >= l && i < count - 1) {
			entries[i].key = entries[i + 1].key;
			entries[i].value = entries[i + 1].value;
		}

		if (lone_lisp_table_is_used(indexes, i) && indexes[i] >= l) {
			--indexes[i];
		}
	}

	entries[count - 1].key = lone_lisp_nil();
	entries[count - 1].value = lone_lisp_nil();

	--actual->count;
}

struct lone_lisp_value lone_lisp_table_key_at(struct lone_lisp *lone, struct lone_lisp_value table, lone_size i)
{
	return lone_lisp_heap_value_of(lone, table)->as.table.entries[i].key;
}

struct lone_lisp_value lone_lisp_table_value_at(struct lone_lisp *lone, struct lone_lisp_value table, lone_size i)
{
	return lone_lisp_heap_value_of(lone, table)->as.table.entries[i].value;
}
