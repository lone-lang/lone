/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MEMORY_ALLOCATOR_HEADER
#define LONE_MEMORY_ALLOCATOR_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

#include <stdalign.h>

void *lone_memory_allocate(struct lone_system *system,
                           size_t count, size_t size,
                           size_t alignment,
                           enum lone_memory_allocation_flags flags);

void *lone_memory_reallocate(struct lone_system *system,
                             void *pointer,
                             size_t old_count, size_t old_size,
                             size_t new_count, size_t new_size,
                             size_t alignment,
                             enum lone_memory_allocation_flags flags);

void lone_memory_deallocate(struct lone_system *system,
                            void *pointer,
                            size_t count, size_t size,
                            size_t alignment);

#endif /* LONE_MEMORY_ALLOCATOR_HEADER */
