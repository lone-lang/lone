/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/memory/allocator.h>

#include <lone/lisp/heap.h>

#include <lone/bits.h>
#include <lone/linux.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Calculates bitmap size for a given heap value capacity.             │
   │    Aligns to unsigned long to maximize performance                     │
   │    of the lone_bits_find_first_zero function.                          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
size_t lone_lisp_heap_bitmap_size(size_t capacity)
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

static intptr_t lone_lisp_heap_remap_bitmap(void **pointer, size_t old_size, size_t new_size)
{
	intptr_t remapped;

	if (!pointer || !*pointer) { return -1; }

	remapped = lone_lisp_heap_mremap(*pointer, old_size, new_size);
	if (remapped < 0) { return remapped; }
	*pointer = (void *) remapped;
	return remapped;
}

static intptr_t lone_lisp_heap_remap_bitmaps(struct lone_lisp_heap *heap, size_t old_size, size_t new_size)
{
	intptr_t remapped;

	remapped = lone_lisp_heap_remap_bitmap(&heap->bits.live, old_size, new_size);
	if (remapped < 0) { return remapped; }

	remapped = lone_lisp_heap_remap_bitmap(&heap->bits.marked, old_size, new_size);
	if (remapped < 0) { return remapped; }

	remapped = lone_lisp_heap_remap_bitmap(&heap->bits.pinned, old_size, new_size);
	if (remapped < 0) { return remapped; }

	return remapped;
}

static intptr_t lone_lisp_heap_remap_values(struct lone_lisp_heap_value **values, size_t old_size, size_t new_size)
{
	intptr_t remapped;

	if (!values || !*values) { return -1; }

	remapped = lone_lisp_heap_mremap(*values, old_size, new_size);
	if (remapped < 0) { return remapped; }
	*values = (struct lone_lisp_heap_value *) remapped;
	return remapped;
}

static void lone_lisp_heap_grow(struct lone_lisp *lone)
{
	size_t old_values_size, new_values_size;
	size_t old_bitmap_size, new_bitmap_size;
	size_t new_capacity;

	if (__builtin_mul_overflow(lone->heap.capacity, LONE_LISP_HEAP_GROWTH_FACTOR, &new_capacity)) { goto overflow; }
	if (new_capacity > ((size_t) 1 << LONE_LISP_INDEX_BITS)) { goto overflow; }
	if (__builtin_mul_overflow(new_capacity, sizeof(struct lone_lisp_heap_value), &new_values_size)) { goto overflow; }

	old_values_size = lone->heap.capacity * sizeof(struct lone_lisp_heap_value);
	old_bitmap_size = lone_lisp_heap_bitmap_size(lone->heap.capacity);
	new_bitmap_size = lone_lisp_heap_bitmap_size(new_capacity);

	if (lone_lisp_heap_remap_values(&lone->heap.values, old_values_size, new_values_size) < 0) {
		goto remap_error;
	}

	if (lone_lisp_heap_remap_bitmaps(&lone->heap, old_bitmap_size, new_bitmap_size) < 0) {
		goto remap_error;
	}

	lone->heap.capacity = new_capacity;

	return;

overflow:
remap_error:
	linux_exit(-1);
}

struct lone_lisp_heap_value *lone_lisp_heap_allocate_value(struct lone_lisp *lone)
{
	size_t bitmap_bytes, byte_offset, i;
	struct lone_optional_size result;
	unsigned char *start;

	bitmap_bytes = lone_lisp_heap_bitmap_size(lone->heap.capacity);
	byte_offset = lone->heap.first_dead / CHAR_BIT;

retry:

	start = (unsigned char *) lone->heap.bits.live + byte_offset;

	result = lone_bits_find_first_zero(start, bitmap_bytes - byte_offset);

	if (!result.present) {
		/* all bits from first_dead to capacity are set: heap is full */
		lone_lisp_heap_grow(lone);
		bitmap_bytes = lone_lisp_heap_bitmap_size(lone->heap.capacity);
		/* new pages are zero filled: live = 0
		 * next scan will find those values */
		goto retry;
	}

	i = byte_offset * CHAR_BIT + result.value;

	if (i >= lone->heap.count) {
		lone->heap.count = i + 1;
	}

	lone_bits_mark(lone->heap.bits.live, i);
	lone->heap.first_dead = i + 1;

	return &lone->heap.values[i];
}

static intptr_t lone_lisp_heap_initialize_values(struct lone_lisp_heap_value **values, size_t size)
{
	intptr_t mapped;

	if (!values) { return -1; }

	/* anonymous pages are zero-filled: live = marked = 0 for all values */
	mapped = lone_lisp_heap_mmap(size);
	if (mapped < 0) { return mapped; }
	*values = (struct lone_lisp_heap_value *) mapped;

	return mapped;
}

static intptr_t lone_lisp_heap_initialize_bitmap(void **bitmap, size_t size)
{
	intptr_t mapped;

	if (!bitmap) { return -1; }

	mapped = lone_lisp_heap_mmap(size);
	if (mapped < 0) { return mapped; }
	*bitmap = (void *) mapped;

	return mapped;
}

static intptr_t lone_lisp_heap_initialize_bitmaps(struct lone_lisp_heap *heap, size_t size)
{
	intptr_t mapped;

	mapped = lone_lisp_heap_initialize_bitmap(&heap->bits.live, size);
	if (mapped < 0) { return mapped; }

	mapped = lone_lisp_heap_initialize_bitmap(&heap->bits.marked, size);
	if (mapped < 0) { return mapped; }

	mapped = lone_lisp_heap_initialize_bitmap(&heap->bits.pinned, size);
	if (mapped < 0) { return mapped; }

	return mapped;
}

void lone_lisp_heap_initialize(struct lone_lisp *lone)
{
	size_t values_size, bitmap_size;

	if (__builtin_mul_overflow(LONE_LISP_HEAP_INITIAL_CAPACITY, sizeof(struct lone_lisp_heap_value), &values_size)) {
		goto error;
	}

	bitmap_size = lone_lisp_heap_bitmap_size(LONE_LISP_HEAP_INITIAL_CAPACITY);

	if (lone_lisp_heap_initialize_values(&lone->heap.values, values_size) < 0) {
		goto error;
	}


	if (lone_lisp_heap_initialize_bitmaps(&lone->heap, bitmap_size) < 0) {
		goto error;
	}

	lone->heap.count = 0;
	lone->heap.capacity = LONE_LISP_HEAP_INITIAL_CAPACITY;
	lone->heap.first_dead = 0;

	return;

error:
	linux_exit(-1);
}
