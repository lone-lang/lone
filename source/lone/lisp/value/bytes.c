/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/value.h>
#include <lone/lisp/value/bytes.h>
#include <lone/lisp/heap.h>

#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

struct lone_lisp_value lone_lisp_bytes_transfer(struct lone_lisp *lone,
		unsigned char *pointer, size_t count,
		bool should_deallocate)
{
	struct lone_lisp_heap_value *actual = lone_lisp_heap_allocate_value(lone);
	actual->type = LONE_LISP_TYPE_BYTES;
	actual->as.bytes.count = count;
	actual->as.bytes.pointer = pointer;
	actual->should_deallocate_bytes = should_deallocate;
	return lone_lisp_value_from_heap_value(actual);
}

struct lone_lisp_value lone_lisp_bytes_transfer_bytes(struct lone_lisp *lone,
		struct lone_bytes bytes, bool should_deallocate)
{
	return lone_lisp_bytes_transfer(lone, bytes.pointer, bytes.count, should_deallocate);
}

struct lone_lisp_value lone_lisp_bytes_copy(struct lone_lisp *lone, unsigned char *pointer, size_t count)
{
	unsigned char *copy = lone_allocate_uninitialized(lone->system, count + 1);
	lone_memory_move(pointer, copy, count);
	copy[count] = '\0';
	return lone_lisp_bytes_transfer(lone, copy, count, true);
}

struct lone_lisp_value lone_lisp_bytes_create(struct lone_lisp *lone, size_t count)
{
	unsigned char *pointer = lone_allocate(lone->system, count + 1);
	return lone_lisp_bytes_transfer(lone, pointer, count, true);
}
