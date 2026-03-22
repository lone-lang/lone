/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/memory/allocator.h>

#include <lone/lisp/heap.h>

#include <lone/linux.h>

static void lone_lisp_heap_grow(struct lone_lisp *lone)
{
	size_t new_size, new_capacity;
	size_t old_size;
	intptr_t remapped;

	if (__builtin_mul_overflow(lone->heap.capacity, LONE_LISP_HEAP_GROWTH_FACTOR, &new_capacity)) { goto overflow; }
	if (__builtin_mul_overflow(new_capacity, sizeof(struct lone_lisp_heap_value), &new_size)) { goto overflow; }

	old_size = lone->heap.capacity * sizeof(struct lone_lisp_heap_value);

	remapped = linux_mremap(lone->heap.values, old_size, new_size, MREMAP_MAYMOVE, 0);
	if (remapped < 0) { goto mremap_error; }

	lone->heap.values = (struct lone_lisp_heap_value *) remapped;
	lone->heap.capacity = new_capacity;

	return;

overflow:
mremap_error:
	linux_exit(-1);
}

struct lone_lisp_heap_value *lone_lisp_heap_allocate_value(struct lone_lisp *lone)
{
	struct lone_lisp_heap_value *element;
	size_t i;

	for (i = lone->heap.first_dead; i < lone->heap.count; ++i) {
		element = &lone->heap.values[i];

		if (!element->live) {
			goto resurrect;
		}
	}

	/* no dead values to resurrect */

	if (lone->heap.count >= lone->heap.capacity) {
		/* invalidates all pointers to lone_lisp_heap_values */
		lone_lisp_heap_grow(lone);
	}

	element = &lone->heap.values[lone->heap.count++];

resurrect:
	element->live = true;
	lone->heap.first_dead = i + 1;
	return element;
}

void lone_lisp_heap_initialize(struct lone_lisp *lone)
{
	intptr_t memory;
	size_t size;

	if (__builtin_mul_overflow(LONE_LISP_HEAP_INITIAL_CAPACITY, sizeof(struct lone_lisp_heap_value), &size)) {
		goto error;
	}

	/* anonymous pages are zero-filled: live = 0 for all values */
	memory = linux_mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (memory < 0) { goto error; }

	lone->heap.values = (struct lone_lisp_heap_value *) memory;
	lone->heap.count = 0;
	lone->heap.capacity = LONE_LISP_HEAP_INITIAL_CAPACITY;
	lone->heap.first_dead = 0;

	return;

error:
	linux_exit(-1);
}
