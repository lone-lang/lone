/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/linux.h>
#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>
#include <lone/utilities.h>

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
		new->next = block->next;
		new->prev = block;
		new->free = 1;
		new->size = excess - sizeof(struct lone_memory);
		block->next = new;
		block->size -= excess + sizeof(struct lone_memory);
	}
}

static void lone_memory_coalesce(struct lone_memory *block)
{
	struct lone_memory *next;

	if (block && block->free) {
		next = block->next;
		if (next && next->free) {
			block->size += next->size + sizeof(struct lone_memory);
			next = block->next = next->next;
			if (next) { next->prev = block; }
		}
	}
}

static struct lone_memory * lone_memory_find_free_block(struct lone_system *system, size_t requested_size, size_t alignment)
{
	size_t needed_size = requested_size + sizeof(struct lone_memory);
	struct lone_memory *block;

	needed_size = lone_align(needed_size, alignment);

	for (block = system->memory; block; block = block->next) {
		if (block->free && block->size >= needed_size)
			break;
	}

	if (!block) { linux_exit(-1); }

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
	struct lone_memory *old = ((struct lone_memory *) pointer) - 1,
	                   *new = ((struct lone_memory *) lone_allocate(system, size)) - 1;

	if (pointer) {
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
