/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/value.h>
#include <lone/memory/garbage_collector.h>

struct lone_value *lone_value_create(struct lone_lisp *lone)
{
	return lone_allocate_from_heap(lone);
}
