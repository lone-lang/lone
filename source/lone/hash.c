/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/hash.h>
#include <lone/hash/fnv_1a.h>

#include <lone/linux.h>

void lone_hash_initialize(struct lone_lisp *lone, struct lone_bytes random)
{
	lone_hash_fnv_1a_initialize(lone, random);
}

static size_t lone_hash_heap_value_recursively(struct lone_heap_value *value, unsigned long hash);

static size_t lone_hash_value_recursively(struct lone_value value, unsigned long hash)
{
	struct lone_bytes bytes;

	bytes.pointer = (unsigned char *) &value.type;
	bytes.count = sizeof(value.type);
	hash = lone_hash_fnv_1a(bytes, hash);

	switch (value.type) {
	case LONE_TYPE_NIL:
		return hash;
	case LONE_TYPE_INTEGER:
		bytes.pointer = (unsigned char *) &value.as.signed_integer;
		bytes.count = sizeof(value.as.signed_integer);
		break;
	case LONE_TYPE_POINTER:
		bytes.pointer = (unsigned char *) &value.as.pointer;
		bytes.count = sizeof(value.as.pointer);
		break;
	case LONE_TYPE_HEAP_VALUE:
		return lone_hash_heap_value_recursively(value.as.heap_value, hash);
	}

	hash = lone_hash_fnv_1a(bytes, hash);
	return hash;
}

static size_t lone_hash_heap_value_recursively(struct lone_heap_value *value, unsigned long hash)
{
	struct lone_bytes bytes;

	bytes.pointer = (unsigned char *) &value->type;
	bytes.count = sizeof(value->type);
	hash = lone_hash_fnv_1a(bytes, hash);

	switch (value->type) {
	case LONE_TYPE_MODULE:
	case LONE_TYPE_FUNCTION:
	case LONE_TYPE_PRIMITIVE:
	case LONE_TYPE_VECTOR:
	case LONE_TYPE_TABLE:
		linux_exit(-1);
	case LONE_TYPE_LIST:
		hash = lone_hash_value_recursively(value->as.list.first, hash);
		hash = lone_hash_value_recursively(value->as.list.rest, hash);
		return hash;
	case LONE_TYPE_SYMBOL:
		bytes.pointer = (unsigned char *) &value;
		bytes.count = sizeof(value);
		break;
	case LONE_TYPE_TEXT:
	case LONE_TYPE_BYTES:
		bytes = value->as.bytes;
		break;
	}

	hash = lone_hash_fnv_1a(bytes, hash);
	return hash;
}


size_t lone_hash(struct lone_lisp *lone, struct lone_value value)
{
	return lone_hash_value_recursively(value, lone->hash.fnv_1a.offset_basis);
}
