/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>
#include <lone/lisp/heap.h>
#include <lone/lisp/hash.h>

#include <lone/memory/allocator.h>
#include <lone/memory/array.h>
#include <lone/memory/functions.h>

#include <lone/utilities.h>

static void lone_lisp_table_allocate_hash_storage(struct lone_system *system,
		size_t capacity, size_t **indexes, struct lone_lisp_table_entry **entries)
{
	*indexes = lone_memory_array(
		system,
		0,
		0,
		capacity,
		sizeof(**indexes),
		alignof(**indexes)
	);

	lone_memory_one(*indexes, lone_memory_array_size_in_bytes(capacity, sizeof(**indexes)));

	*entries = lone_memory_array(
		system,
		0,
		0,
		capacity,
		sizeof(**entries),
		alignof(**entries)
	);
}

struct lone_lisp_value lone_lisp_table_create(struct lone_lisp *lone,
		size_t capacity, struct lone_lisp_value prototype)
{
	struct lone_lisp_heap_value *heap_value;
	struct lone_lisp_table *actual;

	heap_value = lone_lisp_heap_allocate_value(lone);
	actual = &heap_value->as.table;

	capacity = lone_next_power_of_2(capacity);

	/* Fibonacci hashing computes hash_to_index as:
	 *
	 * 	(hash * FIBONACCI_CONSTANT) >> (BITS_PER_LONG - ctz(capacity))
	 *
	 * Capacities are always clamped to 2.
	 * When capacity is 1, ctz(1) is 0,
	 * producing a shift of BITS_PER_LONG,
	 * which is undefined behavior in C
	 * for types of that width. Capacity 1
	 * is also completely pointless:
	 * the load factor check triggers
	 * a resize immediately on the
	 * first insertion.
	 */
	if (capacity < 2) { capacity = 2; }

	actual->prototype = prototype;
	actual->count     = 0;
	actual->capacity  = capacity;
	actual->hash.used = 0;

	lone_lisp_table_allocate_hash_storage(
		lone->system,
		capacity,
		&actual->hash.indexes,
		&actual->hash.entries
	);

	return lone_lisp_value_from_heap_value(lone, heap_value, LONE_LISP_TAG_TABLE);
}

struct lone_lisp_value lone_lisp_table_create_from_shape(struct lone_lisp *lone,
		struct lone_lisp_value shape, struct lone_lisp_value prototype)
{
	struct lone_lisp_heap_value *heap_value;
	struct lone_lisp_shape *actual_shape;
	struct lone_lisp_table *actual_table;

	heap_value   = lone_lisp_heap_allocate_value(lone);
	actual_table = &heap_value->as.table;
	actual_shape = &lone_lisp_heap_value_of(lone, shape)->as.shape;

	actual_table->prototype = prototype;
	actual_table->count     = actual_shape->count;
	actual_table->capacity  = 0;

	actual_table->shaped.shape  = shape;
	actual_table->shaped.values = lone_memory_array(
		lone->system,
		0,
		0,
		actual_shape->count,
		sizeof(*actual_table->shaped.values),
		alignof(*actual_table->shaped.values)
	);

	heap_value->shaped = true;

	return lone_lisp_value_from_heap_value(lone, heap_value, LONE_LISP_TAG_TABLE);
}

static bool lone_lisp_table_is_shaped(struct lone_lisp *lone, struct lone_lisp_value table)
{
	return lone_lisp_heap_value_of(lone, table)->shaped;
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

/* Fibonacci hashing constant = 2^N / φ
 *
 * Multiplying a hash by this and extracting the high bits
 * distributes table entries across table slots better,
 * eliminating primary clustering from correlated hashes.
 *
 * Knuth, The Art of Computer Programming, Volume 3, §6.4.
 */
#if __BITS_PER_LONG == 64
	#define LONE_LISP_TABLE_FIBONACCI_CONSTANT 11400714819323198485UL
#elif __BITS_PER_LONG == 32
	#define LONE_LISP_TABLE_FIBONACCI_CONSTANT 2654435769UL
#else
	#error "Unsupported architecture"
#endif

static unsigned long lone_lisp_table_hash_to_index(lone_hash hash, size_t capacity)
{
	return (hash * LONE_LISP_TABLE_FIBONACCI_CONSTANT) >> (__BITS_PER_LONG - __builtin_ctzl(capacity));
}

#undef LONE_LISP_TABLE_FIBONACCI_CONSTANT

static unsigned long lone_lisp_table_compute_hash_for(struct lone_lisp *lone,
		struct lone_lisp_value key, size_t capacity)
{
	return lone_lisp_table_hash_to_index(lone_lisp_hash_of(lone, key), capacity);
}

static bool lone_lisp_table_key_matches(struct lone_lisp *lone,
		struct lone_lisp_value stored, struct lone_lisp_value key,
		lone_hash key_hash)
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
	 * Compare cached hashes before the structural check
	 * to skip the heap walk on the common case where
	 * two probed entries differ. Equal values always
	 * produce the same hash, so hash mismatches are
	 * definitive negatives.
	 */
	if (stored.tagged == key.tagged) { return true; }

	switch (lone_lisp_type_of(key)) {
	case LONE_LISP_TAG_LIST:
	case LONE_LISP_TAG_TEXT:
	case LONE_LISP_TAG_BYTES:
		if (lone_lisp_hash_of(lone, stored) != key_hash) { return false; }
		return lone_lisp_is_equal(lone, stored, key);
	default:
		return false;
	}
}

static size_t lone_lisp_table_entry_find_index_for(struct lone_lisp *lone, struct lone_lisp_value key,
		size_t *indexes, struct lone_lisp_table_entry *entries, size_t capacity)
{
	lone_hash key_hash;
	size_t i;

	key_hash = lone_lisp_hash_of(lone, key);
	i        = lone_lisp_table_hash_to_index(key_hash, capacity);

	while (    lone_lisp_table_is_used(indexes, i)
	       && !lone_lisp_table_key_matches(lone, entries[indexes[i]].key, key, key_hash)) {

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
		lone_hash hash, struct lone_bytes bytes, enum lone_lisp_tag type,
		size_t *indexes, struct lone_lisp_table_entry *entries, size_t capacity)
{
	unsigned char hash_bits = (unsigned char) hash;
	size_t i = lone_lisp_table_hash_to_index(hash, capacity);

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
	old_used     = actual->hash.used;
	old_entries  = actual->hash.entries;
	old_indexes  = actual->hash.indexes;

	lone_lisp_table_allocate_hash_storage(
		lone->system,
		new_capacity,
		&new_indexes,
		&new_entries
	);

	for (i = 0, j = 0; i < old_used; ++i) {
		if (lone_lisp_is_tombstone(old_entries[i].key)) { continue; }

		new_entries[j].key   = old_entries[i].key;
		new_entries[j].value = old_entries[i].value;

		index = lone_lisp_table_entry_find_index_for(lone,
				new_entries[j].key, new_indexes, new_entries, new_capacity);
		new_indexes[index] = j;
		++j;
	}

	lone_memory_deallocate(lone->system, old_indexes, old_capacity, sizeof(*old_indexes), alignof(*old_indexes));
	lone_memory_deallocate(lone->system, old_entries, old_capacity, sizeof(*old_entries), alignof(*old_entries));

	actual->hash.indexes = new_indexes;
	actual->hash.entries = new_entries;
	actual->capacity     = new_capacity;
	actual->hash.used    = j;
}

/* Convert a shaped table to normal hash table.
 * Moves the data from table.shaped to table.hash
 * and clears the shaped bit on the heap value.
 */
static void lone_lisp_table_deoptimize(struct lone_lisp *lone, struct lone_lisp_value table)
{
	struct lone_lisp_heap_value *heap_value;
	struct lone_lisp_table_entry *new_entries;
	struct lone_lisp_value *old_values;
	struct lone_lisp_table *actual;
	struct lone_lisp_shape *shape;
	size_t count, capacity, i, index;
	size_t *new_indexes;

	/* safe because system-level allocations
	   cannot trigger lone's garbage collector */
	heap_value = lone_lisp_heap_value_of(lone, table);
	actual     = &heap_value->as.table;
	shape      = &lone_lisp_heap_value_of(lone, actual->shaped.shape)->as.shape;
	old_values = actual->shaped.values;
	count      = shape->count;
	capacity   = lone_next_power_of_2(count + 1);

	/* Fibonacci hashing requires capacity >= 2 */
	if (capacity < 2) { capacity = 2; }

	lone_lisp_table_allocate_hash_storage(
		lone->system,
		capacity,
		&new_indexes,
		&new_entries
	);

	actual->capacity     = capacity;
	actual->hash.indexes = new_indexes;
	actual->hash.entries = new_entries;
	actual->hash.used    = 0;

	for (i = 0; i < count; ++i) {
		index = lone_lisp_table_entry_find_index_for(
			lone,
			shape->keys[i],
			actual->hash.indexes,
			actual->hash.entries,
			capacity
		);

		actual->hash.indexes[index]                   = actual->hash.used;
		actual->hash.entries[actual->hash.used].key   = shape->keys[i];
		actual->hash.entries[actual->hash.used].value = old_values[i];
		++actual->hash.used;
	}

	heap_value->shaped = false;

	lone_memory_deallocate(
		lone->system,
		old_values,
		count,
		sizeof(*old_values),
		alignof(*old_values)
	);
}

static bool lone_lisp_table_shape_key_matches(struct lone_lisp *lone,
		struct lone_lisp_value stored, struct lone_lisp_value key)
{
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

static void lone_lisp_table_hash_set(struct lone_lisp *lone, struct lone_lisp_value table,
		struct lone_lisp_value key, struct lone_lisp_value value)
{
	struct lone_lisp_table *actual;
	size_t i, new_capacity;
	bool resize;

	actual = &lone_lisp_heap_value_of(lone, table)->as.table;
	i = lone_lisp_table_entry_find_index_for(lone, key,
			actual->hash.indexes, actual->hash.entries, actual->capacity);

	if (lone_lisp_table_is_used(actual->hash.indexes, i)) {
		actual->hash.entries[actual->hash.indexes[i]].value = value;
	} else {
		resize = lone_lisp_table_needs_resize(lone, table, 1);
		if (actual->hash.used >= actual->capacity || resize) {
			new_capacity = resize? actual->capacity * LONE_LISP_TABLE_GROWTH_FACTOR : actual->capacity;
			lone_lisp_table_resize(lone, table, new_capacity);
			actual = &lone_lisp_heap_value_of(lone, table)->as.table;

			i = lone_lisp_table_entry_find_index_for(
				lone,
				key,
				actual->hash.indexes,
				actual->hash.entries,
				actual->capacity
			);
		}

		actual->hash.indexes[i] = actual->hash.used;
		actual->hash.entries[actual->hash.used].key = key;
		actual->hash.entries[actual->hash.used].value = value;
		++actual->hash.used;
		++actual->count;
	}
}

void lone_lisp_table_set(struct lone_lisp *lone, struct lone_lisp_value table,
		struct lone_lisp_value key, struct lone_lisp_value value)
{
	struct lone_lisp_table *actual;
	struct lone_lisp_shape *shape;
	size_t i;

	if (lone_lisp_table_is_shaped(lone, table)) {
		actual = &lone_lisp_heap_value_of(lone, table)->as.table;
		shape  = &lone_lisp_heap_value_of(lone, actual->shaped.shape)->as.shape;

		for (i = 0; i < shape->count; ++i) {
			if (lone_lisp_table_shape_key_matches(lone, shape->keys[i], key)) {
				actual->shaped.values[i] = value;
				return;
			}
		}

		lone_lisp_table_deoptimize(lone, table);
		lone_lisp_table_hash_set(lone, table, key, value);
		return;
	}

	lone_lisp_table_hash_set(lone, table, key, value);
}

struct lone_lisp_value lone_lisp_table_get(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_lisp_value key)
{
	struct lone_lisp_table *actual;
	struct lone_lisp_shape *shape;
	struct lone_lisp_table_entry *entries;
	size_t *indexes;
	size_t capacity, i;

	actual = &lone_lisp_heap_value_of(lone, table)->as.table;

	if (lone_lisp_table_is_shaped(lone, table)) {
		shape = &lone_lisp_heap_value_of(lone, actual->shaped.shape)->as.shape;

		for (i = 0; i < shape->count; ++i) {
			if (lone_lisp_table_shape_key_matches(lone, shape->keys[i], key)) {
				return actual->shaped.values[i];
			}
		}

		if (!lone_lisp_is_nil(actual->prototype)) {
			return lone_lisp_table_get(lone, actual->prototype, key);
		}

		return lone_lisp_nil();
	}

	indexes  = actual->hash.indexes;
	entries  = actual->hash.entries;
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
		lone_hash hash, struct lone_bytes bytes, enum lone_lisp_tag type)
{
	struct lone_lisp_heap_value *heap_value;
	struct lone_lisp_table_entry *entries;
	struct lone_lisp_table *actual;
	struct lone_lisp_shape *shape;
	unsigned char hash_bits;
	size_t capacity, i;
	size_t *indexes;

	heap_value = lone_lisp_heap_value_of(lone, table);
	actual     = &heap_value->as.table;

	if (heap_value->shaped) {
		shape     = &lone_lisp_heap_value_of(lone, actual->shaped.shape)->as.shape;
		hash_bits = (unsigned char) hash;

		for (i = 0; i < shape->count; ++i) {
			if (lone_lisp_table_bytes_is_equal(lone, shape->keys[i], bytes, type, hash_bits)) {
				return actual->shaped.values[i];
			}
		}
	} else {
		indexes  = actual->hash.indexes;
		entries  = actual->hash.entries;
		capacity = actual->capacity;

		i = lone_lisp_table_entry_find_index_by(lone, hash, bytes, type, indexes, entries, capacity);

		if (lone_lisp_table_is_used(indexes, i)) {
			return entries[indexes[i]].value;
		}
	}

	if (!lone_lisp_is_nil(actual->prototype)) {
		return lone_lisp_table_get_by(lone, actual->prototype, hash, bytes, type);
	}

	return lone_lisp_nil();
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

	if (lone_lisp_table_is_shaped(lone, table)) {
		lone_lisp_table_deoptimize(lone, table);
	}

	actual = &lone_lisp_heap_value_of(lone, table)->as.table;
	indexes = actual->hash.indexes;
	entries = actual->hash.entries;
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

bool lone_lisp_table_next_entry(struct lone_lisp *lone,
		struct lone_lisp_value table, size_t *i,
		struct lone_lisp_table_entry *out)
{
	struct lone_lisp_heap_value *heap_value;
	struct lone_lisp_table *actual;
	struct lone_lisp_shape *shape;

	heap_value = lone_lisp_heap_value_of(lone, table);
	actual     = &heap_value->as.table;

	if (heap_value->shaped) {
		shape = &lone_lisp_heap_value_of(lone, actual->shaped.shape)->as.shape;

		if (*i < shape->count) {
			out->key   = shape->keys[*i];
			out->value = actual->shaped.values[*i];
			return true;
		}
	} else {
		while (*i < actual->hash.used) {
			if (!lone_lisp_is_tombstone(actual->hash.entries[*i].key)) {
				out->key   = actual->hash.entries[*i].key;
				out->value = actual->hash.entries[*i].value;
				return true;
			}
			++(*i);
		}
	}

	return false;
}
