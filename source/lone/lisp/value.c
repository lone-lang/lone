/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/value.h>
#include <lone/lisp/heap.h>

struct lone_lisp_value lone_lisp_value_from_heap_value(struct lone_lisp_heap_value *heap_value)
{
	return (struct lone_lisp_value) {
		.tagged = (long) heap_value,
	};
}
