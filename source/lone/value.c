/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/value.h>
#include <lone/memory/heap.h>

struct lone_value lone_value_from_heap_value(struct lone_heap_value *heap_value)
{
	return (struct lone_value) {
		.type = LONE_HEAP_VALUE,
		.as.heap_value = heap_value
	};
}
