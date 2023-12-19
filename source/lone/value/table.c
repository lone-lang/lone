/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/constants.h>
#include <lone/hash.h>
#include <lone/value.h>
#include <lone/value/table.h>

#include <lone/memory/allocator.h>
#include <lone/memory/heap.h>
#include <lone/memory/array.h>

struct lone_value lone_table_create(struct lone_lisp *lone, size_t capacity, struct lone_value prototype)
{
	struct lone_heap_value *heap_value = lone_heap_allocate_value(lone);
	struct lone_table *actual = &heap_value->as.table;

	heap_value->type = LONE_TABLE;
	actual->prototype = prototype;
	actual->count = 0;
	actual->capacity = capacity;
	actual->indexes = lone_memory_array(lone, 0, actual->capacity, sizeof(*actual->indexes));
	actual->entries = lone_memory_array(lone, 0, actual->capacity, sizeof(*actual->entries));

	return lone_value_from_heap_value(heap_value);
}

size_t lone_table_count(struct lone_value table)
{
	return table.as.heap_value->as.table.count;
}

static double lone_table_load_factor(struct lone_value table, unsigned char added)
{
	struct lone_table *actual = &table.as.heap_value->as.table;

	double count = (double) (actual->count + added);
	double capacity = (double) actual->capacity;

	return count / capacity;
}

static unsigned long lone_table_compute_hash_for(struct lone_lisp *lone, struct lone_value key, size_t capacity)
{
	return lone_hash(lone, key) % capacity;
}

static size_t lone_table_entry_find_index_for(struct lone_lisp *lone, struct lone_value key, struct lone_table_index *indexes, struct lone_table_entry *entries, size_t capacity)
{
	size_t i = lone_table_compute_hash_for(lone, key, capacity);

	while (indexes[i].used && !lone_is_equal(entries[indexes[i].index].key, key)) {
		i = (i + 1) % capacity;
	}

	return i;
}

static bool lone_table_entry_set(struct lone_lisp *lone, struct lone_table_index *indexes, struct lone_table_entry *entries, size_t capacity, size_t index_if_new_entry, struct lone_value key, struct lone_value value)
{
	size_t i = lone_table_entry_find_index_for(lone, key, indexes, entries, capacity);

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

static void lone_table_resize(struct lone_lisp *lone, struct lone_value table, size_t new_capacity)
{
	struct lone_table *actual;
	struct lone_table_index *old_indexes, *new_indexes;
	struct lone_table_entry *old_entries, *new_entries;
	size_t old_capacity;
	size_t i;

	actual = &table.as.heap_value->as.table;

	old_capacity = actual->capacity;
	old_entries  = actual->entries;
	old_indexes  = actual->indexes;

	new_indexes = lone_memory_array(lone,           0, new_capacity, sizeof(*new_indexes));
	new_entries = lone_memory_array(lone, old_entries, new_capacity, sizeof(*new_entries));

	for (i = 0; i < old_capacity; ++i) {
		if (old_indexes[i].used) {
			lone_table_entry_set(
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

	lone_deallocate(lone, old_indexes);

	actual->indexes = new_indexes;
	actual->entries = new_entries;
	actual->capacity = new_capacity;
}

void lone_table_set(struct lone_lisp *lone, struct lone_value table, struct lone_value key, struct lone_value value)
{
	struct lone_table *actual;
	bool is_new_table_entry;

	actual = &table.as.heap_value->as.table;

	if (lone_table_load_factor(table, 1) > LONE_TABLE_LOAD_FACTOR) {
		lone_table_resize(lone, table, actual->capacity * LONE_TABLE_GROWTH_FACTOR);
	}

	is_new_table_entry = lone_table_entry_set(
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

struct lone_value lone_table_get(struct lone_lisp *lone, struct lone_value table, struct lone_value key)
{
	struct lone_table *actual;
	struct lone_table_index *indexes;
	struct lone_table_entry *entries;
	size_t capacity, i;

	actual = &table.as.heap_value->as.table;
	indexes = actual->indexes;
	entries = actual->entries;
	capacity = actual->capacity;

	i = lone_table_entry_find_index_for(lone, key, indexes, entries, capacity);

	if (indexes[i].used) {
		return entries[indexes[i].index].value;
	} else if (!lone_is_nil(actual->prototype)) {
		return lone_table_get(lone, actual->prototype, key);
	} else {
		return lone_nil();
	}
}

void lone_table_delete(struct lone_lisp *lone, struct lone_value table, struct lone_value key)
{
	struct lone_table *actual;
	struct lone_table_index *indexes;
	struct lone_table_entry *entries;
	size_t capacity, count;
	size_t i, j, k, l;

	actual = &table.as.heap_value->as.table;
	indexes = actual->indexes;
	entries = actual->entries;
	capacity = actual->capacity;
	count = actual->count;

	i = lone_table_entry_find_index_for(lone, key, indexes, entries, capacity);

	if (!indexes[i].used) { return; }

	l = indexes[i].index;

	j = i;
	while (1) {
		j = (j + 1) % capacity;
		if (!indexes[j].used) { break; }
		k = lone_table_compute_hash_for(lone, entries[indexes[j].index].key, capacity);
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

	entries[count].key = lone_nil();
	entries[count].value = lone_nil();

	--actual->count;
}
