/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/constants.h>
#include <lone/lisp/hash.h>
#include <lone/lisp/value.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/heap.h>

#include <lone/memory/allocator.h>
#include <lone/memory/array.h>

struct lone_lisp_value lone_lisp_table_create(struct lone_lisp *lone,
		size_t capacity, struct lone_lisp_value prototype)
{
	struct lone_lisp_heap_value *heap_value = lone_lisp_heap_allocate_value(lone);
	struct lone_lisp_table *actual = &heap_value->as.table;

	heap_value->type = LONE_LISP_TYPE_TABLE;
	actual->prototype = prototype;
	actual->count = 0;
	actual->capacity = capacity;
	actual->indexes = lone_memory_array(lone->system, 0, actual->capacity, sizeof(*actual->indexes));
	actual->entries = lone_memory_array(lone->system, 0, actual->capacity, sizeof(*actual->entries));

	return lone_lisp_value_from_heap_value(heap_value);
}

size_t lone_lisp_table_count(struct lone_lisp_value table)
{
	return lone_lisp_value_to_heap_value(table)->as.table.count;
}

static double lone_lisp_table_load_factor(struct lone_lisp_value table, unsigned char added)
{
	struct lone_lisp_table *actual = &lone_lisp_value_to_heap_value(table)->as.table;

	double count = (double) (actual->count + added);
	double capacity = (double) actual->capacity;

	return count / capacity;
}

static unsigned long lone_lisp_table_compute_hash_for(struct lone_lisp *lone,
		struct lone_lisp_value key, size_t capacity)
{
	return lone_lisp_hash(lone, key) % capacity;
}

static size_t lone_lisp_table_entry_find_index_for(struct lone_lisp *lone, struct lone_lisp_value key,
		struct lone_lisp_table_index *indexes, struct lone_lisp_table_entry *entries,
		size_t capacity)
{
	size_t i = lone_lisp_table_compute_hash_for(lone, key, capacity);

	while (indexes[i].used && !lone_lisp_is_equal(entries[indexes[i].index].key, key)) {
		i = (i + 1) % capacity;
	}

	return i;
}

static bool lone_lisp_table_entry_set(struct lone_lisp *lone,
		struct lone_lisp_table_index *indexes, struct lone_lisp_table_entry *entries,
		size_t capacity, size_t index_if_new_entry,
		struct lone_lisp_value key, struct lone_lisp_value value)
{
	size_t i = lone_lisp_table_entry_find_index_for(lone, key, indexes, entries, capacity);

	if (indexes[i].used) {
		entries[indexes[i].index].value = value;
		return false;
	} else {
		indexes[i].used = true;
		indexes[i].index = index_if_new_entry;
		entries[indexes[i].index].key = key;
		entries[indexes[i].index].value = value;
		return true;
	}
}

static void lone_lisp_table_resize(struct lone_lisp *lone, struct lone_lisp_value table, size_t new_capacity)
{
	struct lone_lisp_table *actual;
	struct lone_lisp_table_index *old_indexes, *new_indexes;
	struct lone_lisp_table_entry *old_entries, *new_entries;
	size_t old_capacity;
	size_t i;

	actual = &lone_lisp_value_to_heap_value(table)->as.table;

	old_capacity = actual->capacity;
	old_entries  = actual->entries;
	old_indexes  = actual->indexes;

	new_indexes = lone_memory_array(lone->system,           0, new_capacity, sizeof(*new_indexes));
	new_entries = lone_memory_array(lone->system, old_entries, new_capacity, sizeof(*new_entries));

	for (i = 0; i < old_capacity; ++i) {
		if (old_indexes[i].used) {
			lone_lisp_table_entry_set(
				lone,
				new_indexes,
				new_entries,
				new_capacity,
				old_indexes[i].index,
				new_entries[old_indexes[i].index].key,
				new_entries[old_indexes[i].index].value
			);
		}
	}

	lone_deallocate(lone->system, old_indexes);

	actual->indexes = new_indexes;
	actual->entries = new_entries;
	actual->capacity = new_capacity;
}

void lone_lisp_table_set(struct lone_lisp *lone, struct lone_lisp_value table,
		struct lone_lisp_value key, struct lone_lisp_value value)
{
	struct lone_lisp_table *actual;
	bool is_new_table_entry;

	actual = &lone_lisp_value_to_heap_value(table)->as.table;

	if (lone_lisp_table_load_factor(table, 1) > LONE_LISP_TABLE_LOAD_FACTOR) {
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
	struct lone_lisp_table_index *indexes;
	struct lone_lisp_table_entry *entries;
	size_t capacity, i;

	actual = &lone_lisp_value_to_heap_value(table)->as.table;
	indexes = actual->indexes;
	entries = actual->entries;
	capacity = actual->capacity;

	i = lone_lisp_table_entry_find_index_for(lone, key, indexes, entries, capacity);

	if (indexes[i].used) {
		return entries[indexes[i].index].value;
	} else if (!lone_lisp_is_nil(actual->prototype)) {
		return lone_lisp_table_get(lone, actual->prototype, key);
	} else {
		return lone_lisp_nil();
	}
}

void lone_lisp_table_delete(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_lisp_value key)
{
	struct lone_lisp_table *actual;
	struct lone_lisp_table_index *indexes;
	struct lone_lisp_table_entry *entries;
	size_t capacity, count;
	size_t i, j, k, l;

	actual = &lone_lisp_value_to_heap_value(table)->as.table;
	indexes = actual->indexes;
	entries = actual->entries;
	capacity = actual->capacity;
	count = actual->count;

	i = lone_lisp_table_entry_find_index_for(lone, key, indexes, entries, capacity);

	if (!indexes[i].used) { return; }

	l = indexes[i].index;

	j = i;
	while (1) {
		j = (j + 1) % capacity;
		if (!indexes[j].used) { break; }
		k = lone_lisp_table_compute_hash_for(lone, entries[indexes[j].index].key, capacity);
		if ((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j))) {
			indexes[i].used = indexes[j].used;
			indexes[i].index = indexes[j].index;
			i = j;
		}
	}

	indexes[i].used = false;
	indexes[i].index = 0;

	for (i = 0; i < capacity; ++i) {

		if (i >= l && i < count - 1) {
			entries[i].key = entries[i + 1].key;
			entries[i].value = entries[i + 1].value;
		}

		if (indexes[i].used && indexes[i].index >= l) {
			--indexes[i].index;
		}
	}

	entries[count].key = lone_lisp_nil();
	entries[count].value = lone_lisp_nil();

	--actual->count;
}

struct lone_lisp_value lone_lisp_table_key_at(struct lone_lisp_value table, lone_size i)
{
	return lone_lisp_value_to_heap_value(table)->as.table.entries[i].key;
}

struct lone_lisp_value lone_lisp_table_value_at(struct lone_lisp_value table, lone_size i)
{
	return lone_lisp_value_to_heap_value(table)->as.table.entries[i].value;
}
