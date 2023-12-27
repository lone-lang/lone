/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MEMORY_ARRAY_HEADER
#define LONE_MEMORY_ARRAY_HEADER

#include <lone/types.h>

size_t lone_memory_array_size_in_bytes(size_t element_count, size_t element_size);
bool lone_memory_array_is_bounded(size_t element_index, size_t element_capacity, size_t element_size);
void * lone_memory_array(struct lone_lisp *lone, void *buffer_or_null, size_t element_count, size_t element_size);

#endif /* LONE_MEMORY_ARRAY_HEADER */
