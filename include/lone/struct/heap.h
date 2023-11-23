/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_STRUCT_HEAP_HEADER
#define LONE_STRUCT_HEAP_HEADER

#include <lone/types.h>

struct lone_heap {
	struct lone_heap *next;
	size_t count;
	struct lone_value values[];
};

#endif /* LONE_STRUCT_HEAP_HEADER */
