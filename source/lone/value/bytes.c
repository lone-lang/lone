#include <lone/value.h>
#include <lone/value/bytes.h>
#include <lone/memory/allocator.h>
#include <lone/memory/functions.h>

#include <lone/struct/value.h>

struct lone_value *lone_bytes_transfer(struct lone_lisp *lone, unsigned char *pointer, size_t count, bool should_deallocate)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_BYTES;
	value->bytes.count = count;
	value->bytes.pointer = pointer;
	value->should_deallocate_bytes = should_deallocate;
	return value;
}

struct lone_value *lone_bytes_transfer_bytes(struct lone_lisp *lone, struct lone_bytes bytes, bool should_deallocate)
{
	return lone_bytes_transfer(lone, bytes.pointer, bytes.count, should_deallocate);
}

struct lone_value *lone_bytes_create(struct lone_lisp *lone, unsigned char *pointer, size_t count)
{
	unsigned char *copy = lone_allocate(lone, count + 1);
	lone_memory_move(pointer, copy, count);
	copy[count] = '\0';
	return lone_bytes_transfer(lone, copy, count, true);
}
