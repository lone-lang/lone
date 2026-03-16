/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/memory/allocator.h>

#include <lone/lisp/heap.h>

#include <lone/linux.h>

struct lone_lisp_heap_value *lone_lisp_heap_allocate_value(struct lone_lisp *lone)
{
	struct lone_lisp_heap_value *element;
	size_t i;

	for (i = 0; i < lone->heap.count; ++i) {
		element = &lone->heap.values[i];

		if (!element->live) {
			goto resurrect;
		}
	}

	/* no dead values to resurrect */

	if (lone->heap.count >= lone->heap.capacity) {
		/* all available pages exhausted */
		goto out_of_memory;
	}

	element = &lone->heap.values[lone->heap.count++];

resurrect:
	element->live = true;
	return element;

out_of_memory:
	linux_exit(-1);
}

void lone_lisp_heap_initialize(struct lone_lisp *lone)
{
	intptr_t memory;
	size_t size;

	if (__builtin_mul_overflow(LONE_LISP_HEAP_CAPACITY, sizeof(struct lone_lisp_heap_value), &size)) { goto error; }

	/* anonymous pages are zero-filled: live = 0 for all values */
	memory = linux_mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	if (memory < 0) { goto error; }

	lone->heap.values = (struct lone_lisp_heap_value *) memory;
	lone->heap.count = 0;
	lone->heap.capacity = LONE_LISP_HEAP_CAPACITY;

	return;

error:
	linux_exit(-1);
}
