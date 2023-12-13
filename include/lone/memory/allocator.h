/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MEMORY_ALLOCATOR_HEADER
#define LONE_MEMORY_ALLOCATOR_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

size_t
__attribute__((const))
lone_align(size_t size, size_t alignment);

void *
__attribute__((malloc, alloc_size(2), alloc_align(3)))
lone_allocate_aligned(struct lone_lisp *lone, size_t requested_size, size_t alignment);

void *
__attribute__((malloc, alloc_size(2), alloc_align(3)))
lone_allocate_aligned_uninitialized(struct lone_lisp *lone, size_t requested_size, size_t alignment);

void *
__attribute__((malloc, alloc_size(2), assume_aligned(LONE_ALIGNMENT)))
lone_allocate(struct lone_lisp *lone, size_t requested_size);

void *
__attribute__((malloc, alloc_size(2), assume_aligned(LONE_ALIGNMENT)))
lone_allocate_uninitialized(struct lone_lisp *lone, size_t requested_size);

void *
__attribute__((alloc_size(3)))
lone_reallocate(struct lone_lisp *lone, void *pointer, size_t size);

void lone_deallocate(struct lone_lisp *lone, void *pointer);

#endif /* LONE_MEMORY_ALLOCATOR_HEADER */
