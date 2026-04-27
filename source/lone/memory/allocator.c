/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/linux.h>
#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>
#include <lone/utilities.h>

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
	copy_size = old_total < new_total? old_total : new_total;
	lone_memory_move(pointer, block, copy_size);
	lone_memory_deallocate(system, pointer, old_count, old_size, alignment);
	return block;

overflow:
	linux_exit(-1);
}

void lone_memory_deallocate(struct lone_system *system,
                            void *pointer,
                            size_t count, size_t size,
                            size_t alignment)
{
	size_t total, class, class_size;
	struct lone_memory_slab *slab;

	if (__builtin_mul_overflow(count, size, &total)) { goto overflow; }

	if (total > LONE_MEMORY_SLAB_MAX) {
		lone_memory_munmap_rounded(pointer, total, system->allocator.page_size);
		return;
	}

	class = lone_memory_effective_class(total, alignment);

	if (class >= LONE_MEMORY_SLAB_CLASSES) {
		lone_memory_munmap_rounded(pointer, total, system->allocator.page_size);
		return;
	}

	class_size = LONE_MEMORY_SLAB_MIN << class;
	slab = &system->allocator.slabs[class];

	lone_memory_zero(pointer, class_size);
	*(void **) pointer = slab->free;
	slab->free = pointer;
	return;

overflow:
	linux_exit(-1);
}
