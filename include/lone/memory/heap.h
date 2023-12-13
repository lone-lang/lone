/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MEMORY_HEAP_HEADER
#define LONE_MEMORY_HEAP_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

void lone_heap_initialize(struct lone_lisp *lone);
struct lone_heap_value *lone_heap_allocate_value(struct lone_lisp *lone);
void lone_deallocate_dead_heaps(struct lone_lisp *lone);

#endif /* LONE_MEMORY_HEAP_HEADER */
