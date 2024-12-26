/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/hash.h>
#include <lone/lisp/types.h>

#include <lone/hash/fnv_1a.h>

#include <lone/linux.h>

static size_t lone_lisp_hash_heap_value_recursively(struct lone_lisp_heap_value *value, unsigned long hash);

static size_t lone_lisp_hash_value_recursively(struct lone_lisp_value value, unsigned long hash)
{
	struct lone_bytes bytes;
	enum lone_lisp_value_type type;
	lone_lisp_integer integer;

	type = lone_lisp_type_of(value);

	bytes.pointer = (unsigned char *) &type;
	bytes.count = sizeof(type);
	hash = lone_hash_fnv_1a(bytes, hash);

	switch (type) {
	case LONE_LISP_TYPE_NIL:
		return hash;
	case LONE_LISP_TYPE_HEAP_VALUE:
		return lone_lisp_hash_heap_value_recursively(lone_lisp_heap_value_of(value), hash);
	case LONE_LISP_TYPE_INTEGER:
		integer = lone_lisp_integer_of(value);
		bytes.pointer = (unsigned char *) &integer;
		bytes.count = sizeof(integer);
		break;
	}

	hash = lone_hash_fnv_1a(bytes, hash);
	return hash;
}

static size_t lone_lisp_hash_heap_value_recursively(struct lone_lisp_heap_value *value, unsigned long hash)
{
	struct lone_bytes bytes;

	bytes.pointer = (unsigned char *) &value->type;
	bytes.count = sizeof(value->type);
	hash = lone_hash_fnv_1a(bytes, hash);

	switch (value->type) {
	case LONE_LISP_TYPE_MODULE:
	case LONE_LISP_TYPE_FUNCTION:
	case LONE_LISP_TYPE_PRIMITIVE:
	case LONE_LISP_TYPE_VECTOR:
	case LONE_LISP_TYPE_TABLE:
		linux_exit(-1);
	case LONE_LISP_TYPE_LIST:
		hash = lone_lisp_hash_value_recursively(value->as.list.first, hash);
		hash = lone_lisp_hash_value_recursively(value->as.list.rest, hash);
		return hash;
	case LONE_LISP_TYPE_SYMBOL:
		bytes.pointer = (unsigned char *) &value;
		bytes.count = sizeof(value);
		break;
	case LONE_LISP_TYPE_TEXT:
	case LONE_LISP_TYPE_BYTES:
		bytes = value->as.bytes;
		break;
	}

	hash = lone_hash_fnv_1a(bytes, hash);
	return hash;
}

size_t lone_lisp_hash(struct lone_lisp *lone, struct lone_lisp_value value)
{
	return lone_lisp_hash_value_recursively(value, lone->system->hash.fnv_1a.offset_basis);
}
