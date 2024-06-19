/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_HEAP_HEADER
#define LONE_LISP_HEAP_HEADER

#include <lone/lisp/definitions.h>
#include <lone/lisp/types.h>

void lone_lisp_heap_initialize(struct lone_lisp *lone);
struct lone_lisp_heap_value *lone_lisp_heap_allocate_value(struct lone_lisp *lone);
void lone_lisp_deallocate_dead_heaps(struct lone_lisp *lone);

#endif /* LONE_LISP_HEAP_HEADER */
