/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MEMORY_HEADER
#define LONE_MEMORY_HEADER

#include <lone/types.h>

void lone_memory_initialize(
	struct lone_lisp *lone,
	struct lone_bytes memory,
	size_t heap_size,
	void *stack
);

#endif /* LONE_MEMORY_HEADER */
