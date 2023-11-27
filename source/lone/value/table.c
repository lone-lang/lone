/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/constants.h>
#include <lone/hash.h>
#include <lone/value.h>
#include <lone/value/table.h>

#include <lone/memory/allocator.h>

struct lone_value *lone_table_create(struct lone_lisp *lone, size_t capacity, struct lone_value *prototype)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_TABLE;
	value->table.prototype = prototype;
	value->table.capacity = capacity;
	value->table.count = 0;
	value->table.indexes = lone_allocate(lone, capacity * sizeof(*value->table.indexes));
	value->table.entries = lone_allocate(lone, capacity * sizeof(*value->table.entries));

	for (size_t i = 0; i < capacity; ++i) {
		value->table.indexes[i].used = false;
		value->table.indexes[i].index = 0;
		value->table.entries[i].key = 0;
		value->table.entries[i].value = 0;
	}

	return value;
}

static unsigned long lone_table_compute_hash_for(struct lone_lisp *lone, struct lone_value *key, size_t capacity)
{
	return lone_hash(lone, key) % capacity;
}

static size_t lone_table_entry_find_index_for(struct lone_lisp *lone, struct lone_value *key, struct lone_table_index *indexes, struct lone_table_entry *entries, size_t capacity)
{
	size_t i = lone_table_compute_hash_for(lone, key, capacity);

	while (indexes[i].used && !lone_is_equal(entries[indexes[i].index].key, key)) {
		i = (i + 1) % capacity;
	}

	return i;
}

static int lone_table_entry_set(struct lone_lisp *lone, struct lone_table_index *indexes, struct lone_table_entry *entries, size_t capacity, size_t index_if_new_entry, struct lone_value *key, struct lone_value *value)
{
	size_t i = lone_table_entry_find_index_for(lone, key, indexes, entries, capacity);

	if (indexes[i].used) {
		entries[indexes[i].index].value = value;
		return 0;
	} else {
		indexes[i].used = true;
		indexes[i].index = index_if_new_entry;
		entries[indexes[i].index].key = key;
		entries[indexes[i].index].value = value;
		return 1;
	}
}

static void lone_table_resize(struct lone_lisp *lone, struct lone_value *table, size_t new_capacity)
{
	struct lone_table_index *old_indexes, *new_indexes;
	struct lone_table_entry *old_entries, *new_entries;
	size_t old_capacity;
	size_t i;

	old_indexes = table->table.indexes;
	new_indexes = lone_allocate(lone, new_capacity * sizeof(*new_indexes));

	old_entries = table->table.entries;
	new_entries = lone_allocate(lone, new_capacity * sizeof(*new_entries));

	old_capacity = table->table.capacity;

	for (i = 0; i < new_capacity; ++i) {
		new_indexes[i].used = false;
		new_indexes[i].index = 0;
		new_entries[i].key = 0;
		new_entries[i].value = 0;
	}

	for (i = 0; i < old_capacity; ++i) {
		if (old_indexes[i].used) {
			lone_table_entry_set(
				lone,
				new_indexes,
				new_entries,
				new_capacity,
				old_indexes[i].index,
				old_entries[old_indexes[i].index].key,
				old_entries[old_indexes[i].index].value
			);
		}
	}

	lone_deallocate(lone, old_indexes);
	lone_deallocate(lone, old_entries);
	table->table.indexes = new_indexes;
	table->table.entries = new_entries;
	table->table.capacity = new_capacity;
}

void lone_table_set(struct lone_lisp *lone, struct lone_value *table, struct lone_value *key, struct lone_value *value)
{
	int is_new_table_entry;

	if (table->table.count >= table->table.capacity / 2) {
		lone_table_resize(lone, table, table->table.capacity * 2);
	}

	is_new_table_entry = lone_table_entry_set(
		lone,
		table->table.indexes,
		table->table.entries,
		table->table.capacity,
		table->table.count,
		key,
		value
	);

	if (is_new_table_entry) {
		++table->table.count;
	}
}

struct lone_value *lone_table_get(struct lone_lisp *lone, struct lone_value *table, struct lone_value *key)
{
	struct lone_table_index *indexes = table->table.indexes;
	struct lone_table_entry *entries = table->table.entries, *entry;
	struct lone_value *prototype = table->table.prototype;
	size_t capacity = table->table.capacity, i;

	i = lone_table_entry_find_index_for(lone, key, indexes, entries, capacity);

	if (indexes[i].used) {
		return entries[indexes[i].index].value;
	} else if (prototype && !lone_is_nil(prototype)) {
		return lone_table_get(lone, prototype, key);
	} else {
		return lone_nil(lone);
	}
}

void lone_table_delete(struct lone_lisp *lone, struct lone_value *table, struct lone_value *key)
{
	struct lone_table_index *indexes = table->table.indexes;
	struct lone_table_entry *entries = table->table.entries;
	size_t capacity = table->table.capacity, count = table->table.count;
	size_t i, j, k, l;

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

	entries[count].key = 0;
	entries[count].value = 0;

	--table->table.count;
}
