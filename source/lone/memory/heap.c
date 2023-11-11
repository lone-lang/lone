/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/memory/heap.h>
#include <lone/memory/allocator.h>

#include <lone/struct/lisp.h>
#include <lone/struct/heap.h>

static struct lone_heap *lone_allocate_heap(struct lone_lisp *lone, size_t count)
{
	size_t i, size = sizeof(struct lone_heap) + (sizeof(struct lone_value) * count);
	struct lone_heap *heap = lone_allocate(lone, size);
	heap->next = 0;
	heap->count = count;
	for (i = 0; i < count; ++i) {
		heap->values[i].live = false;
		heap->values[i].marked = false;
	}
	return heap;
}

static struct lone_value *lone_allocate_from_heap(struct lone_lisp *lone)
{
	struct lone_value *element;
	struct lone_heap *heap, *prev;
	size_t i;

	for (prev = lone->memory.heaps, heap = prev; heap; prev = heap, heap = heap->next) {
		for (i = 0; i < heap->count; ++i) {
			element = &heap->values[i];

			if (!element->live) {
				goto resurrect;
			}
		}
	}

	heap = lone_allocate_heap(lone, lone->memory.heaps[0].count);
	prev->next = heap;
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
		for (i = 0; i < heap->count; ++i) {
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

struct lone_value *lone_heap_allocate_value(struct lone_lisp *lone)
{
	return lone_allocate_from_heap(lone);
}

void lone_heap_initialize(struct lone_lisp *lone, size_t heap_size)
{
	lone->memory.heaps = lone_allocate_heap(lone, heap_size);
}
