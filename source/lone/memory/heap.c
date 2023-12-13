/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/memory/heap.h>
#include <lone/memory/allocator.h>

struct lone_heap_value *lone_heap_allocate_value(struct lone_lisp *lone)
{
	struct lone_heap_value *element;
	struct lone_heap *heap, *prev;
	size_t i;

	for (prev = lone->memory.heaps, heap = prev; heap; prev = heap, heap = heap->next) {
		for (i = 0; i < LONE_MEMORY_HEAP_VALUE_COUNT; ++i) {
			element = &heap->values[i];

			if (!element->live) {
				goto resurrect;
			}
		}
	}

	heap = lone_allocate(lone, sizeof(struct lone_heap));
	prev->next = heap;
	heap->next = 0;
	element = &heap->values[0];

resurrect:
	element->live = true;
	return element;
}

void lone_deallocate_dead_heaps(struct lone_lisp *lone)
{
	struct lone_heap *prev = lone->memory.heaps, *heap = prev->next;
	size_t i;

	while (heap) {
		for (i = 0; i < LONE_MEMORY_HEAP_VALUE_COUNT; ++i) {
			if (heap->values[i].live) { /* at least one live object */ goto next_heap; }
		}

		/* no live objects */
		prev->next = heap->next;
		lone_deallocate(lone, heap);
		heap = prev->next;
		continue;
next_heap:
		prev = heap;
		heap = heap->next;
	}
}

void lone_heap_initialize(struct lone_lisp *lone)
{
	lone->memory.heaps = lone_allocate(lone, sizeof(struct lone_heap));
}
