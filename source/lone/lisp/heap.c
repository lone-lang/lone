/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/memory/allocator.h>

#include <lone/lisp/heap.h>

#include <lone/bits.h>
#include <lone/linux.h>

#include <limits.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Calculates bitmap size for a given heap value capacity.             │
   │    Aligns to unsigned long to maximize performance                     │
   │    of the lone_bits_find_first_zero function.                          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static size_t lone_lisp_heap_bitmap_size(size_t capacity)
{
	size_t bytes = (capacity + CHAR_BIT - 1) / CHAR_BIT;
	return (bytes + sizeof(unsigned long) - 1) & ~(sizeof(unsigned long) - 1);
}

static intptr_t lone_lisp_heap_mmap(size_t size)
{
	return linux_mmap(0, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}

static intptr_t lone_lisp_heap_mremap(void *address, size_t old_size, size_t new_size)
{
	return linux_mremap(address, old_size, new_size, MREMAP_MAYMOVE, 0);
}

static void lone_lisp_heap_grow(struct lone_lisp *lone)
{
	size_t old_values_size, new_values_size;
	size_t old_bitmap_size, new_bitmap_size;
	size_t new_capacity;
	intptr_t remapped;

	if (__builtin_mul_overflow(lone->heap.capacity, LONE_LISP_HEAP_GROWTH_FACTOR, &new_capacity)) { goto overflow; }
	if (__builtin_mul_overflow(new_capacity, sizeof(struct lone_lisp_heap_value), &new_values_size)) { goto overflow; }

	old_values_size = lone->heap.capacity * sizeof(struct lone_lisp_heap_value);
	old_bitmap_size = lone_lisp_heap_bitmap_size(lone->heap.capacity);
	new_bitmap_size = lone_lisp_heap_bitmap_size(new_capacity);

	/* grow the values array */
	remapped = lone_lisp_heap_mremap(lone->heap.values, old_values_size, new_values_size);
	if (remapped < 0) { goto mremap_error; }
	lone->heap.values = (struct lone_lisp_heap_value *) remapped;

	/* grow the live bitmap */
	remapped = lone_lisp_heap_mremap(lone->heap.bits.live, old_bitmap_size, new_bitmap_size);
	if (remapped < 0) { goto mremap_error; }
	lone->heap.bits.live = (void *) remapped;

	/* grow the marked bitmap */
	remapped = lone_lisp_heap_mremap(lone->heap.bits.marked, old_bitmap_size, new_bitmap_size);
	if (remapped < 0) { goto mremap_error; }
	lone->heap.bits.marked = (void *) remapped;

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
		if (!lone_bits_get(lone->heap.bits.live, i)) {
			element = &lone->heap.values[i];
			goto resurrect;
		}
	}

	/* no dead values to resurrect */

	if (lone->heap.count >= lone->heap.capacity) {
		/* invalidates all pointers to lone_lisp_heap_values */
		lone_lisp_heap_grow(lone);
	}

	i = lone->heap.count++;
	element = &lone->heap.values[i];

resurrect:
	lone_bits_mark(lone->heap.bits.live, i);
	lone->heap.first_dead = i + 1;
	return element;
}

void lone_lisp_heap_initialize(struct lone_lisp *lone)
{
	size_t values_size, bitmap_size;
	intptr_t memory;

	if (__builtin_mul_overflow(LONE_LISP_HEAP_INITIAL_CAPACITY, sizeof(struct lone_lisp_heap_value), &values_size)) {
		goto error;
	}

	bitmap_size = lone_lisp_heap_bitmap_size(LONE_LISP_HEAP_INITIAL_CAPACITY);

	/* anonymous pages are zero-filled: live = marked = 0 for all values */
	memory = lone_lisp_heap_mmap(values_size);
	if (memory < 0) { goto error; }
	lone->heap.values = (struct lone_lisp_heap_value *) memory;

	memory = lone_lisp_heap_mmap(bitmap_size);
	if (memory < 0) { goto error; }
	lone->heap.bits.live = (void *) memory;

	memory = lone_lisp_heap_mmap(bitmap_size);
	if (memory < 0) { goto error; }
	lone->heap.bits.marked = (void *) memory;

	lone->heap.count = 0;
	lone->heap.capacity = LONE_LISP_HEAP_INITIAL_CAPACITY;
	lone->heap.first_dead = 0;

	return;

error:
	linux_exit(-1);
}
