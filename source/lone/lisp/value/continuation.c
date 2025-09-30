/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/value.h>
#include <lone/lisp/value/continuation.h>

#include <lone/lisp/heap.h>

struct lone_lisp_value lone_lisp_continuation_create(struct lone_lisp *lone,
		size_t frame_count, struct lone_lisp_machine_stack_frame *frames)
{
	struct lone_lisp_heap_value *actual = lone_lisp_heap_allocate_value(lone);
	actual->type = LONE_LISP_TYPE_CONTINUATION;
	actual->as.continuation.frame_count = frame_count;
	actual->as.continuation.frames = frames;
	return lone_lisp_value_from_heap_value(actual);
}
