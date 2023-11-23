/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/linux.h>
#include <lone/value.h>
#include <lone/value/pointer.h>
#include <lone/value/integer.h>

struct lone_value *lone_pointer_create(struct lone_lisp *lone, void *pointer, enum lone_pointer_type type)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_POINTER;
	value->pointer.type = type;
	value->pointer.address = pointer;
	return value;
}

struct lone_value *lone_pointer_dereference(struct lone_lisp *lone, struct lone_value *pointer)
{
	enum lone_pointer_type type;
	void *address;

	if (!lone_is_pointer(pointer)) { /* can't dereference this value */ linux_exit(-1); }

	type = pointer->pointer.type;
	address = pointer->pointer.address;

	switch (type) {
	case LONE_TO_U8:
		return lone_integer_create(lone, *((uint8_t *) address));
	case LONE_TO_I8:
		return lone_integer_create(lone, *((int8_t *) address));
	case LONE_TO_U16:
		return lone_integer_create(lone, *((uint16_t *) address));
	case LONE_TO_I16:
		return lone_integer_create(lone, *((int16_t *) address));
	case LONE_TO_U32:
		return lone_integer_create(lone, *((uint32_t *) address));
	case LONE_TO_I32:
		return lone_integer_create(lone, *((int32_t *) address));
	case LONE_TO_U64:
		return lone_integer_create(lone, (long) *((uint64_t *) address));
	case LONE_TO_I64:
		return lone_integer_create(lone, *((int64_t *) address));
	case LONE_TO_UNKNOWN:
		/* cannot dereference pointer to unknown type */ linux_exit(-1);
	}
}
