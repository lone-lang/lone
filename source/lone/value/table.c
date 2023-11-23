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
	value->table.entries = lone_allocate(lone, capacity * sizeof(*value->table.entries));

	for (size_t i = 0; i < capacity; ++i) {
		value->table.entries[i].key = 0;
		value->table.entries[i].value = 0;
	}

	return value;
}

static unsigned long lone_table_compute_hash_for(struct lone_lisp *lone, struct lone_value *key, size_t capacity)
{
	return lone_hash(lone, key) % capacity;
}

static size_t lone_table_entry_find_index_for(struct lone_lisp *lone, struct lone_value *key, struct lone_table_entry *entries, size_t capacity)
{
	size_t i = lone_table_compute_hash_for(lone, key, capacity);

	while (entries[i].key && !lone_is_equal(entries[i].key, key)) {
		i = (i + 1) % capacity;
	}

	return i;
}

static int lone_table_entry_set(struct lone_lisp *lone, struct lone_table_entry *entries, size_t capacity, struct lone_value *key, struct lone_value *value)
{
	size_t i = lone_table_entry_find_index_for(lone, key, entries, capacity);
	struct lone_table_entry *entry = &entries[i];

	if (entry->key) {
		entry->value = value;
		return 0;
	} else {
		entry->key = key;
		entry->value = value;
		return 1;
	}
}

static void lone_table_resize(struct lone_lisp *lone, struct lone_value *table, size_t new_capacity)
{
	size_t old_capacity = table->table.capacity, i;
	struct lone_table_entry *old = table->table.entries,
	                        *new = lone_allocate(lone, new_capacity * sizeof(*new));

	for (i = 0; i < new_capacity; ++i) {
		new[i].key = 0;
		new[i].value = 0;
	}

	for (i = 0; i < old_capacity; ++i) {
		if (old[i].key) {
			lone_table_entry_set(lone, new, new_capacity, old[i].key, old[i].value);
		}
	}

	lone_deallocate(lone, old);
	table->table.entries = new;
	table->table.capacity = new_capacity;
}

void lone_table_set(struct lone_lisp *lone, struct lone_value *table, struct lone_value *key, struct lone_value *value)
{
	if (table->table.count >= table->table.capacity / 2) {
		lone_table_resize(lone, table, table->table.capacity * 2);
	}

	if (lone_table_entry_set(lone, table->table.entries, table->table.capacity, key, value)) {
		++table->table.count;
	}
}

struct lone_value *lone_table_get(struct lone_lisp *lone, struct lone_value *table, struct lone_value *key)
{
	size_t capacity = table->table.capacity, i;
	struct lone_table_entry *entries = table->table.entries, *entry;
	struct lone_value *prototype = table->table.prototype;

	i = lone_table_entry_find_index_for(lone, key, entries, capacity);
	entry = &entries[i];

	if (entry->key) {
		return entry->value;
	} else if (prototype && !lone_is_nil(prototype)) {
		return lone_table_get(lone, prototype, key);
	} else {
		return lone_nil(lone);
	}
}

void lone_table_delete(struct lone_lisp *lone, struct lone_value *table, struct lone_value *key)
{
	size_t capacity = table->table.capacity, i, j, k;
	struct lone_table_entry *entries = table->table.entries;

	i = lone_table_entry_find_index_for(lone, key, entries, capacity);

	if (!entries[i].key) { return; }

	j = i;
	while (1) {
		j = (j + 1) % capacity;
		if (!entries[j].key) { break; }
		k = lone_table_compute_hash_for(lone, entries[j].key, capacity);
		if ((j > i && (k <= i || k > j)) || (j < i && (k <= i && k > j))) {
			entries[i] = entries[j];
			i = j;
		}
	}

	entries[i].key = 0;
	entries[i].value = 0;
	--table->table.count;
}
