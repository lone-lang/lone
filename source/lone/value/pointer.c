/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/value/pointer.h>
#include <lone/value/integer.h>
#include <lone/linux.h>

struct lone_value lone_pointer_create(void *pointer, enum lone_pointer_type pointer_type)
{
	return (struct lone_value) {
		.type = LONE_TYPE_POINTER,
		.pointer_type = pointer_type,
		.as.pointer.to_void = pointer
	};
}

struct lone_value lone_pointer_dereference(struct lone_value pointer)
{
	if (!lone_is_pointer(pointer)) { /* can't dereference this value */ linux_exit(-1); }

	switch (pointer.pointer_type) {
	case LONE_TO_U8:
		return lone_integer_create(*pointer.as.pointer.to_u8);
	case LONE_TO_S8:
		return lone_integer_create(*pointer.as.pointer.to_s8);
	case LONE_TO_U16:
		return lone_integer_create(*pointer.as.pointer.to_u16);
	case LONE_TO_S16:
		return lone_integer_create(*pointer.as.pointer.to_s16);
	case LONE_TO_U32:
		return lone_integer_create(*pointer.as.pointer.to_u32);
	case LONE_TO_S32:
		return lone_integer_create(*pointer.as.pointer.to_s32);
	case LONE_TO_U64:
		return lone_integer_create(*pointer.as.pointer.to_u64);
	case LONE_TO_S64:
		return lone_integer_create(*pointer.as.pointer.to_s64);
	case LONE_TO_UNKNOWN:
		/* cannot dereference pointer to unknown type */ linux_exit(-1);
	}
}
