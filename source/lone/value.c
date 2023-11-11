/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/value.h>
#include <lone/memory/heap.h>

struct lone_value *lone_value_create(struct lone_lisp *lone)
{
	return lone_heap_allocate_value(lone);
}
