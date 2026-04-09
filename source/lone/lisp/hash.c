/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/hash.h>
#include <lone/lisp/types.h>

#include <lone/hash/fnv_1a.h>

#include <lone/linux.h>

static size_t lone_lisp_hash_value_recursively(struct lone_lisp *lone,
		struct lone_lisp_value value, unsigned long hash)
{
	struct lone_lisp_heap_value *heap_value;
	struct lone_bytes bytes;
	enum lone_lisp_tag tag;
	lone_lisp_integer integer;
	unsigned long symbol_hash;

	tag = lone_lisp_type_of(value);

	bytes.pointer = (unsigned char *) &tag;
	bytes.count = sizeof(tag);
	hash = lone_hash_fnv_1a(bytes, hash);

	switch (tag) {
	case LONE_LISP_TAG_NIL:
	case LONE_LISP_TAG_TRUE:
	case LONE_LISP_TAG_FALSE:
		return hash;
	case LONE_LISP_TAG_INTEGER:
		integer = lone_lisp_integer_of(value);
		bytes.pointer = (unsigned char *) &integer;
		bytes.count = sizeof(integer);
		break;
	case LONE_LISP_TAG_LIST:
		heap_value = lone_lisp_heap_value_of(lone, value);
		hash = lone_lisp_hash_value_recursively(lone, heap_value->as.list.first, hash);
		hash = lone_lisp_hash_value_recursively(lone, heap_value->as.list.rest, hash);
		return hash;
	case LONE_LISP_TAG_SYMBOL:
		if (lone_lisp_is_inline_symbol(value)) {
			struct lone_bytes name = lone_lisp_symbol_name(lone, &value);
			symbol_hash = lone_lisp_hash_as_symbol(lone, name);
		} else {
			symbol_hash = lone_lisp_heap_value_of(lone, value)->as.symbol.hash;
		}
		bytes.pointer = (unsigned char *) &symbol_hash;
		bytes.count = sizeof(symbol_hash);
		break;
	case LONE_LISP_TAG_TEXT:
	case LONE_LISP_TAG_BYTES:
		bytes = lone_lisp_heap_value_of(lone, value)->as.bytes;
		break;
	case LONE_LISP_TAG_MODULE:
	case LONE_LISP_TAG_FUNCTION:
	case LONE_LISP_TAG_PRIMITIVE:
	case LONE_LISP_TAG_CONTINUATION:
	case LONE_LISP_TAG_GENERATOR:
	case LONE_LISP_TAG_VECTOR:
	case LONE_LISP_TAG_TABLE:
		linux_exit(-1);
	}

	hash = lone_hash_fnv_1a(bytes, hash);
	return hash;
}

size_t lone_lisp_hash(struct lone_lisp *lone, struct lone_lisp_value value)
{
	if (lone_lisp_is_symbol(lone, value)) {
		if (lone_lisp_is_inline_symbol(value)) {
			struct lone_bytes name = lone_lisp_symbol_name(lone, &value);
			return lone_lisp_hash_as_symbol(lone, name);
		}
		return lone_lisp_heap_value_of(lone, value)->as.symbol.hash;
	}

	return lone_lisp_hash_value_recursively(lone, value, lone->system->hash.fnv_1a.offset_basis);
}

static size_t lone_lisp_hash_as_tagged_type(struct lone_lisp *lone,
		struct lone_bytes data, enum lone_lisp_tag tag)
{
	struct lone_bytes bytes;
	unsigned long hash;

	hash = lone->system->hash.fnv_1a.offset_basis;

	bytes.pointer = (unsigned char *) &tag;
	bytes.count = sizeof(tag);
	hash = lone_hash_fnv_1a(bytes, hash);

	hash = lone_hash_fnv_1a(data, hash);

	return hash;
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
