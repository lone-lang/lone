/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/value.h>
#include <lone/value/bytes.h>
#include <lone/memory/heap.h>
#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

struct lone_value lone_bytes_transfer(struct lone_lisp *lone, unsigned char *pointer, size_t count, bool should_deallocate)
{
	struct lone_heap_value *actual = lone_heap_allocate_value(lone);
	actual->type = LONE_BYTES;
	actual->as.bytes.count = count;
	actual->as.bytes.pointer = pointer;
	actual->should_deallocate_bytes = should_deallocate;
	return lone_value_from_heap_value(actual);
}

struct lone_value lone_bytes_transfer_bytes(struct lone_lisp *lone, struct lone_bytes bytes, bool should_deallocate)
{
	return lone_bytes_transfer(lone, bytes.pointer, bytes.count, should_deallocate);
}

struct lone_value lone_bytes_copy(struct lone_lisp *lone, unsigned char *pointer, size_t count)
{
	unsigned char *copy = lone_allocate_uninitialized(lone, count + 1);
	lone_memory_move(pointer, copy, count);
	copy[count] = '\0';
	return lone_bytes_transfer(lone, copy, count, true);
}

struct lone_value lone_bytes_create(struct lone_lisp *lone, size_t count)
{
	unsigned char *pointer = lone_allocate(lone, count + 1);
	return lone_bytes_transfer(lone, pointer, count, true);
}
