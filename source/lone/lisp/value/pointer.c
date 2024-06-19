/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/value/pointer.h>
#include <lone/lisp/value/integer.h>

#include <lone/linux.h>

struct lone_lisp_value lone_lisp_pointer_create(void *pointer, enum lone_lisp_pointer_type pointer_type)
{
	return (struct lone_lisp_value) {
		.type = LONE_LISP_TYPE_POINTER,
		.pointer_type = pointer_type,
		.as.pointer.to_void = pointer
	};
}

struct lone_lisp_value lone_lisp_pointer_dereference(struct lone_lisp_value pointer)
{
	if (!lone_lisp_is_pointer(pointer)) {
		/* can't dereference this value */ linux_exit(-1);
	}

	switch (pointer.pointer_type) {
	case LONE_TO_U8:
		return lone_lisp_integer_create(*pointer.as.pointer.to_u8);
	case LONE_TO_S8:
		return lone_lisp_integer_create(*pointer.as.pointer.to_s8);
	case LONE_TO_U16:
		return lone_lisp_integer_create(*pointer.as.pointer.to_u16);
	case LONE_TO_S16:
		return lone_lisp_integer_create(*pointer.as.pointer.to_s16);
	case LONE_TO_U32:
		return lone_lisp_integer_create(*pointer.as.pointer.to_u32);
	case LONE_TO_S32:
		return lone_lisp_integer_create(*pointer.as.pointer.to_s32);
	case LONE_TO_U64:
		return lone_lisp_integer_create(*pointer.as.pointer.to_u64);
	case LONE_TO_S64:
		return lone_lisp_integer_create(*pointer.as.pointer.to_s64);
	case LONE_TO_UNKNOWN:
		/* cannot dereference pointer to unknown type */ linux_exit(-1);
	}
}
