/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>
#include <lone/lisp/heap.h>
#include <lone/lisp/utilities.h>

#include <lone/memory/allocator.h>

struct lone_lisp_value lone_lisp_bytes_transfer(struct lone_lisp *lone,
		unsigned char *pointer, size_t count,
		bool should_deallocate)
{
	struct lone_lisp_heap_value *actual;

	if (count <= LONE_LISP_INLINE_MAX_LENGTH && !should_deallocate) {
		return lone_lisp_inline_bytes_create(pointer, count);
	}

	actual = lone_lisp_heap_allocate_value(lone);
	return lone_lisp_buffer_transfer(lone, actual, &actual->as.bytes.data,
			pointer, count, should_deallocate, LONE_LISP_TAG_BYTES);
}

struct lone_lisp_value lone_lisp_bytes_transfer_bytes(struct lone_lisp *lone,
		struct lone_bytes bytes, bool should_deallocate)
{
	return lone_lisp_bytes_transfer(lone, bytes.pointer, bytes.count, should_deallocate);
}

struct lone_lisp_value lone_lisp_bytes_copy(struct lone_lisp *lone, unsigned char *pointer, size_t count)
{
	struct lone_lisp_heap_value *actual;

	if (count <= LONE_LISP_INLINE_MAX_LENGTH) {
		return lone_lisp_inline_bytes_create(pointer, count);
	}

	actual = lone_lisp_heap_allocate_value(lone);
	return lone_lisp_buffer_copy(lone, actual, &actual->as.bytes.data,
			pointer, count, LONE_LISP_TAG_BYTES);
}

struct lone_lisp_value lone_lisp_bytes_create(struct lone_lisp *lone, size_t count)
{
	unsigned char *pointer = lone_memory_allocate(lone->system, count + 1, 1, 1, LONE_MEMORY_ALLOCATION_FLAGS_NONE);
	return lone_lisp_bytes_transfer(lone, pointer, count, true);
}
