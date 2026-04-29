/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/hash.h>
#include <lone/lisp/types.h>

#include <lone/hash/siphash.h>
#include <lone/types.h>

#include <lone/linux.h>

static void lone_lisp_hash_update_bytes(struct lone_hash_siphash_state *state,
		void *pointer, size_t count)
{
	struct lone_bytes bytes;

	bytes.pointer = pointer;
	bytes.count   = count;

	lone_hash_siphash_update(state, bytes);
}

static void lone_lisp_hash_value_recursively(struct lone_lisp *lone,
		struct lone_lisp_value value, struct lone_hash_siphash_state *state)
{
	struct lone_lisp_heap_value *heap_value;
	enum lone_lisp_tag tag;
	lone_lisp_integer integer;
	unsigned long symbol_hash;

	tag = lone_lisp_type_of(value);

	lone_lisp_hash_update_bytes(state, &tag, sizeof(tag));

	switch (tag) {
	case LONE_LISP_TAG_NIL:
	case LONE_LISP_TAG_TRUE:
	case LONE_LISP_TAG_FALSE:
		return;
	case LONE_LISP_TAG_INTEGER:
		integer = lone_lisp_integer_of(value);
		lone_lisp_hash_update_bytes(state, &integer, sizeof(integer));
		return;
	case LONE_LISP_TAG_LIST:
		heap_value = lone_lisp_heap_value_of(lone, value);
		lone_lisp_hash_value_recursively(lone, heap_value->as.list.first, state);
		lone_lisp_hash_value_recursively(lone, heap_value->as.list.rest, state);
		return;
	case LONE_LISP_TAG_SYMBOL:
		if (lone_lisp_is_inline_symbol(value)) {
			struct lone_bytes name = lone_lisp_bytes_of(lone, &value);
			symbol_hash = lone_lisp_hash_as_symbol(lone, name);
		} else {
			symbol_hash = lone_lisp_heap_value_of(lone, value)->as.symbol.hash;
		}
		lone_lisp_hash_update_bytes(state, &symbol_hash, sizeof(symbol_hash));
		return;
	case LONE_LISP_TAG_TEXT:
		lone_hash_siphash_update(state, lone_lisp_bytes_of(lone, &value));
		return;
	case LONE_LISP_TAG_BYTES:
		if (!lone_lisp_is_frozen(lone, value)) {
			/* mutable bytes cannot be hashed safely */ linux_exit(-1);
		}
		lone_hash_siphash_update(state, lone_lisp_bytes_of(lone, &value));
		return;
	case LONE_LISP_TAG_MODULE:
	case LONE_LISP_TAG_FUNCTION:
	case LONE_LISP_TAG_PRIMITIVE:
	case LONE_LISP_TAG_CONTINUATION:
	case LONE_LISP_TAG_GENERATOR:
	case LONE_LISP_TAG_VECTOR:
	case LONE_LISP_TAG_TABLE:
		linux_exit(-1);
	}
}

static void lone_lisp_hash_initialize_from_system(struct lone_lisp *lone,
		struct lone_hash_siphash_state *state)
{
	lone_u64 k0, k1;

	k0 = lone_u64le_read(lone->system->random);
	k1 = lone_u64le_read(lone->system->random + 8);

	lone_hash_siphash_initialize(state, k0, k1);
}

static unsigned long *hash_of(struct lone_lisp_heap_value *heap_value)
{
	switch (heap_value->type) {
	case LONE_LISP_TAG_SYMBOL: return &heap_value->as.symbol.hash;
	case LONE_LISP_TAG_TEXT:   return &heap_value->as.text.hash;
	case LONE_LISP_TAG_BYTES:  return &heap_value->as.bytes.hash;
	case LONE_LISP_TAG_LIST:   return &heap_value->as.list.hash;
	default:                   /* unhashable type */ linux_exit(-1);
	}
}

size_t lone_lisp_hash(struct lone_lisp *lone, struct lone_lisp_value value)
{
	struct lone_hash_siphash_state state;

	if (lone_lisp_is_symbol(lone, value)) {
		if (lone_lisp_is_inline_symbol(value)) {
			struct lone_bytes name = lone_lisp_bytes_of(lone, &value);
			return lone_lisp_hash_as_symbol(lone, name);
		}
		return lone_lisp_heap_value_of(lone, value)->as.symbol.hash;
	}

	lone_lisp_hash_initialize_from_system(lone, &state);
	lone_lisp_hash_value_recursively(lone, value, &state);

	return lone_hash_siphash_finish(&state);
}

static size_t lone_lisp_hash_as_tagged_type(struct lone_lisp *lone,
		struct lone_bytes data, enum lone_lisp_tag tag)
{
	struct lone_hash_siphash_state state;

	lone_lisp_hash_initialize_from_system(lone, &state);

	lone_lisp_hash_update_bytes(&state, &tag, sizeof(tag));
	lone_hash_siphash_update(&state, data);

	return lone_hash_siphash_finish(&state);
}

size_t lone_lisp_hash_as_symbol(struct lone_lisp *lone, struct lone_bytes name)
{
	return lone_lisp_hash_as_tagged_type(lone, name, LONE_LISP_TAG_SYMBOL);
}

size_t lone_lisp_hash_as_text(struct lone_lisp *lone, struct lone_bytes text)
{
	return lone_lisp_hash_as_tagged_type(lone, text, LONE_LISP_TAG_TEXT);
}

size_t lone_lisp_hash_as_bytes(struct lone_lisp *lone, struct lone_bytes bytes)
{
	return lone_lisp_hash_as_tagged_type(lone, bytes, LONE_LISP_TAG_BYTES);
}

static unsigned long lone_lisp_hash_compute(struct lone_lisp *lone,
		struct lone_lisp_value value)
{
	struct lone_hash_siphash_state state;
	struct lone_bytes bytes;

	switch (lone_lisp_type_of(value)) {
	case LONE_LISP_TAG_SYMBOL:
		bytes = lone_lisp_bytes_of(lone, &value);
		return lone_lisp_hash_as_symbol(lone, bytes);
	case LONE_LISP_TAG_TEXT:
		bytes = lone_lisp_bytes_of(lone, &value);
		return lone_lisp_hash_as_text(lone, bytes);
	case LONE_LISP_TAG_BYTES:
		if (!lone_lisp_is_frozen(lone, value)) {
			/* mutable bytes cannot be hashed safely */ linux_exit(-1);
		}
		bytes = lone_lisp_bytes_of(lone, &value);
		return lone_lisp_hash_as_bytes(lone, bytes);
	case LONE_LISP_TAG_LIST:
		lone_lisp_hash_initialize_from_system(lone, &state);
		lone_lisp_hash_value_recursively(lone, value, &state);
		return lone_hash_siphash_finish(&state);
	default:
		/* unhashable type */ linux_exit(-1);
	}
}

unsigned long lone_lisp_value_compute_and_store_hash(struct lone_lisp *lone,
		struct lone_lisp_value value)
{
	struct lone_lisp_heap_value *heap_value;
	unsigned long hash;

	heap_value = lone_lisp_heap_value_of(lone, value);
	hash       = lone_lisp_hash_compute(lone, value);

	*hash_of(heap_value) = hash;
	heap_value->hash_cached          = true;

	return hash;
}
