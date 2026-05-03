/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>
#include <lone/lisp/heap.h>

#include <lone/memory/array.h>

struct lone_lisp_value lone_lisp_shape_create(struct lone_lisp *lone,
		size_t count, struct lone_lisp_value *keys)
{
	struct lone_lisp_heap_value *heap_value;
	struct lone_lisp_shape *actual;
	size_t i;

	heap_value = lone_lisp_heap_allocate_value(lone);
	actual = &heap_value->as.shape;

	actual->count = count;
	actual->keys = lone_memory_array(
		lone->system,
		0,
		0,
		count,
		sizeof(*actual->keys),
		alignof(*actual->keys)
	);

	for (i = 0; i < count; ++i) {
		actual->keys[i] = keys[i];
	}

	return lone_lisp_value_from_heap_value(lone, heap_value, LONE_LISP_TAG_SHAPE);
}
