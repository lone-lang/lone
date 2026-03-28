/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/linux.h>
#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>
#include <lone/utilities.h>

#include <limits.h>

static inline size_t lone_memory_size_class(size_t size)
{
	size_t log2_ceil;
	if (size <= LONE_MEMORY_SLAB_MIN) { return 0; }
	log2_ceil = (sizeof(size) * CHAR_BIT) - __builtin_clzl(size - 1);
	return log2_ceil - __builtin_ctzl(LONE_MEMORY_SLAB_MIN);
}

static inline size_t lone_memory_effective_class(size_t size, size_t alignment)
{
	size_t class, align_class;
	class = lone_memory_size_class(size);
	align_class = lone_memory_size_class(alignment);
	if (align_class > class) { class = align_class; }
	return class;
}

static inline size_t lone_memory_round_up_to_page(size_t size, size_t page_size)
{
	return (size + page_size - 1) & ~(page_size - 1);
}

static void *lone_memory_mremap(void *pointer, size_t old_size, size_t new_size)
{
	intptr_t remapped;

	remapped = linux_mremap(pointer, old_size, new_size, MREMAP_MAYMOVE, 0);
	if (remapped < 0) { goto mremap_failed; }
	return (void *) remapped;

mremap_failed:
	linux_exit(-1);
}

static void *lone_memory_mmap(size_t size)
{
	intptr_t mapped;

	mapped = linux_mmap(
		0, size,
		PROT_READ | PROT_WRITE,
		MAP_PRIVATE | MAP_ANONYMOUS,
		-1, 0);

	if (mapped < 0) { goto mmap_failed; }
	return (void *) mapped;

mmap_failed:
	linux_exit(-1);
}

static void *lone_memory_mmap_rounded(size_t size, size_t page_size)
{
	return lone_memory_mmap(lone_memory_round_up_to_page(size, page_size));
}

static void lone_memory_munmap(void *pointer, size_t size)
{
	intptr_t unmapped;

	unmapped = linux_munmap(pointer, size);
	if (unmapped < 0) { goto munmap_failed; }
	return;

munmap_failed:
	linux_exit(-1);
}

static void lone_memory_munmap_rounded(void *pointer, size_t size, size_t page_size)
{
	lone_memory_munmap(pointer, lone_memory_round_up_to_page(size, page_size));
}

void *lone_memory_allocate(struct lone_system *system,
                           size_t count, size_t size,
                           size_t alignment,
                           enum lone_memory_allocation_flags flags)
{
	size_t total, class, class_size;
	struct lone_memory_slab *slab;
	unsigned char *block;

	if (__builtin_mul_overflow(count, size, &total)) { goto overflow; }

	if (total > LONE_MEMORY_SLAB_MAX) {
		return lone_memory_mmap_rounded(total, system->allocator.page_size);
	}

	class = lone_memory_effective_class(total, alignment);

	if (class >= LONE_MEMORY_SLAB_CLASSES) {
		return lone_memory_mmap_rounded(total, system->allocator.page_size);
	}

	class_size = LONE_MEMORY_SLAB_MIN << class;
	slab = &system->allocator.slabs[class];

	if (slab->free) {
		block = slab->free;
		slab->free = *(void **) block;
		lone_memory_zero(block, sizeof(void *));
		return block;
	}

	if (slab->position && slab->position + class_size <= slab->end) {
		block = slab->position;
		slab->position += class_size;
		return block;
	}

	block = lone_memory_mmap(LONE_MEMORY_SLAB_SIZE);
	slab->position = block + class_size;
	slab->end = block + LONE_MEMORY_SLAB_SIZE;
	return block;

overflow:
	linux_exit(-1);
}

void *lone_memory_reallocate(struct lone_system *system,
                             void *pointer,
                             size_t old_count, size_t old_size,
                             size_t new_count, size_t new_size,
                             size_t alignment,
                             enum lone_memory_allocation_flags flags)
{
	size_t old_total, new_total, old_class, new_class, old_pages, new_pages, page_size, copy_size;
	void *block;

	if (__builtin_mul_overflow(old_count, old_size, &old_total)) { goto overflow; }
	if (__builtin_mul_overflow(new_count, new_size, &new_total)) { goto overflow; }

	old_class = lone_memory_effective_class(old_total, alignment);
	new_class = lone_memory_effective_class(new_total, alignment);

	if (old_total <= LONE_MEMORY_SLAB_MAX && new_total <= LONE_MEMORY_SLAB_MAX
	    && old_class < LONE_MEMORY_SLAB_CLASSES && new_class < LONE_MEMORY_SLAB_CLASSES
	    && old_class == new_class) {
		return pointer;
	}

	if (old_total > LONE_MEMORY_SLAB_MAX && new_total > LONE_MEMORY_SLAB_MAX) {
		page_size = system->allocator.page_size;
		old_pages = lone_memory_round_up_to_page(old_total, page_size);
		new_pages = lone_memory_round_up_to_page(new_total, page_size);
		if (old_pages == new_pages) { return pointer; }
		return lone_memory_mremap(pointer, old_pages, new_pages);
	}

	block = lone_memory_allocate(system, new_count, new_size, alignment, flags);
	copy_size = old_total < new_total ? old_total : new_total;
	lone_memory_move(pointer, block, copy_size);
	lone_memory_deallocate(system, pointer, old_count, old_size, alignment);
	return block;

overflow:
	linux_exit(-1);
}

static size_t __attribute__((const)) lone_next_power_of_2(size_t n)
{
	size_t next = 1;
	while (next < n) { next *= 2; }
	return next;
}

static size_t __attribute__((const)) lone_next_power_of_2_multiple(size_t n, size_t m)
{
	m = lone_next_power_of_2(m);
	return (n + m - 1) & (~(m - 1));
}

size_t __attribute__((const)) lone_align(size_t size, size_t alignment)
{
	return lone_next_power_of_2_multiple(size, alignment);
}

static void lone_memory_split(struct lone_memory *block, size_t used)
{
	size_t excess = block->size - used;

	/* split block if there's enough space to allocate at least 1 byte */
	if (excess >= sizeof(struct lone_memory) + 1) {
		struct lone_memory *new = (struct lone_memory *) __builtin_assume_aligned(block->pointer + used, LONE_ALIGNMENT);

		if (block->next) { block->next->prev = new; }

		new->next = block->next;
		new->prev = block;
		new->free = 1;
		new->size = excess - sizeof(struct lone_memory);

		block->next = new;
		block->size = used;
	}
}

static bool lone_memory_is_contiguous(struct lone_memory *block, struct lone_memory *next)
{
	return block->pointer + block->size == (unsigned char *) next;
}

static void lone_memory_coalesce(struct lone_memory *block)
{
	struct lone_memory *next;

	if (block && block->free) {
		next = block->next;
		/* only coalesce physically adjacent blocks within the same segment */
		if (next && next->free && lone_memory_is_contiguous(block, next)) {
			block->size += next->size + sizeof(struct lone_memory);
			next = block->next = next->next;
			if (next) { next->prev = block; }
		}
	}
}

static struct lone_memory *lone_memory_map(struct lone_memory *block, size_t needed_size)
{
	size_t block_size, segment_size;
	struct lone_memory *new;
	intptr_t mapped;

	block_size = needed_size + sizeof(struct lone_memory);

	segment_size = block_size < LONE_MEMORY_SEGMENT_MINIMUM_SIZE
	             ? LONE_MEMORY_SEGMENT_MINIMUM_SIZE
	             : block_size;

	mapped = linux_mmap(0, segment_size, PROT_READ | PROT_WRITE,
	                    MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

	if (mapped < 0) { return 0; }

	new = (struct lone_memory *) mapped;
	new->prev = block;
	new->next = 0;
	new->free = 1;
	new->size = segment_size - sizeof(struct lone_memory);

	block->next = new;

	return new;
}

static struct lone_memory * lone_memory_find_free_block(struct lone_system *system, size_t requested_size, size_t alignment)
{
	size_t needed_size;
	struct lone_memory *block, *last;

	needed_size = lone_align(requested_size, alignment);

	for (last = block = system->memory; block; block = block->next) {
		if (block->free && block->size >= needed_size) { break; }
		last = block;
	}

	if (!block) {
		block = lone_memory_map(last, needed_size);
		if (!block) { linux_exit(-1); }
	}

	block->free = 0;
	lone_memory_split(block, needed_size);

	return block;
}

void * lone_allocate_aligned(struct lone_system *system, size_t requested_size, size_t alignment)
{
	struct lone_memory *block = lone_memory_find_free_block(system, requested_size, alignment);
	lone_memory_zero(block->pointer, block->size);
	return block->pointer;
}

void * lone_allocate_aligned_uninitialized(struct lone_system *system, size_t requested_size, size_t alignment)
{
	struct lone_memory *block = lone_memory_find_free_block(system, requested_size, alignment);
	/* zero fill any extra memory allocated due to alignment requirements */
	lone_memory_zero(block->pointer + requested_size, block->size - requested_size);
	return block->pointer;
}

void * lone_allocate(struct lone_system *system, size_t requested_size)
{
	return lone_allocate_aligned(system, requested_size, LONE_ALIGNMENT);
}

void * lone_allocate_uninitialized(struct lone_system *system, size_t requested_size)
{
	return lone_allocate_aligned_uninitialized(system, requested_size, LONE_ALIGNMENT);
}

void * lone_reallocate(struct lone_system *system, void *pointer, size_t size)
{
	struct lone_memory *old, *new;

	new = ((struct lone_memory *) lone_allocate(system, size)) - 1;

	if (pointer) {
		old =  ((struct lone_memory *) pointer) - 1;
		lone_memory_move(old->pointer, new->pointer, lone_min(old->size, new->size));
		lone_deallocate(system, pointer);
	}

	return new->pointer;
}

void lone_deallocate(struct lone_system *system, void *pointer)
{
	struct lone_memory *block = ((struct lone_memory *) pointer) - 1;
	block->free = 1;

	lone_memory_coalesce(block);
	lone_memory_coalesce(block->prev);
}
