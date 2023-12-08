/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>

#ifndef LONE_MEMORY_HEAP_HEADER
#define LONE_MEMORY_HEAP_HEADER

void lone_heap_initialize(struct lone_lisp *lone, size_t heap_size);
struct lone_heap_value *lone_heap_allocate_value(struct lone_lisp *lone);
void lone_deallocate_dead_heaps(struct lone_lisp *lone);

#endif /* LONE_MEMORY_HEAP_HEADER */
