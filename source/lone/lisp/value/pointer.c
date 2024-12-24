/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/value/pointer.h>
#include <lone/lisp/value/integer.h>

#include <lone/linux.h>

struct lone_lisp_value lone_lisp_pointer_create(void *pointer)
{
	return (struct lone_lisp_value) {
		.type = LONE_LISP_TYPE_POINTER,
		.as.pointer = pointer
	};
}
