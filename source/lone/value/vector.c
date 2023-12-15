/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/linux.h>
#include <lone/value.h>
#include <lone/value/vector.h>
#include <lone/value/list.h>
#include <lone/memory/heap.h>
#include <lone/memory/allocator.h>

struct lone_value lone_vector_create(struct lone_lisp *lone, size_t capacity)
{
	struct lone_heap_value *actual = lone_heap_allocate_value(lone);
	actual->type = LONE_VECTOR;
	actual->as.vector.capacity = capacity;
	actual->as.vector.count = 0;
	actual->as.vector.values = lone_allocate(lone, capacity * sizeof(*actual->as.vector.values));

	return lone_value_from_heap_value(actual);
}

void lone_vector_resize(struct lone_lisp *lone, struct lone_value vector, size_t new_capacity)
{
	struct lone_heap_value *actual;
	struct lone_value *new;
	size_t i;

	new = lone_allocate_uninitialized(lone, new_capacity * sizeof(*new));

	actual = vector.as.heap_value;
	for (i = 0; i < new_capacity; ++i) {
		new[i] = i < actual->as.vector.count? actual->as.vector.values[i] : lone_nil();
	}

	lone_deallocate(lone, actual->as.vector.values);

	actual->as.vector.values = new;
	actual->as.vector.capacity = new_capacity;
	if (actual->as.vector.count > new_capacity) { actual->as.vector.count = new_capacity; }
}

struct lone_value lone_vector_get_value_at(struct lone_lisp *lone, struct lone_value vector, size_t i)
{
	struct lone_heap_value *actual = vector.as.heap_value;
	return i < actual->as.vector.capacity? actual->as.vector.values[i] : lone_nil();
}

struct lone_value lone_vector_get(struct lone_lisp *lone, struct lone_value vector, struct lone_value index)
{
	if (!lone_is_integer(index)) { /* only integer indexes supported */ linux_exit(-1); }
	return lone_vector_get_value_at(lone, vector, index.as.unsigned_integer);
}

void lone_vector_set_value_at(struct lone_lisp *lone, struct lone_value vector, size_t i, struct lone_value value)
{
	struct lone_heap_value *actual = vector.as.heap_value;
	if (i >= actual->as.vector.capacity) { lone_vector_resize(lone, vector, i * 2); }
	actual->as.vector.values[i] = value;
	if (++i > actual->as.vector.count) { actual->as.vector.count = i; }
}

void lone_vector_set(struct lone_lisp *lone, struct lone_value vector, struct lone_value index, struct lone_value value)
{
	if (!lone_is_integer(index)) { /* only integer indexes supported */ linux_exit(-1); }
	lone_vector_set_value_at(lone, vector, index.as.unsigned_integer, value);
}

void lone_vector_push(struct lone_lisp *lone, struct lone_value vector, struct lone_value value)
{
	lone_vector_set_value_at(lone, vector, vector.as.heap_value->as.vector.count, value);
}

void lone_vector_push_va_list(struct lone_lisp *lone, struct lone_value vector, size_t count, va_list arguments)
{
	struct lone_value *argument;
	size_t i;

	for (i = 0; i < count; ++i) {
		argument = va_arg(arguments, struct lone_value *);
		lone_vector_push(lone, vector, *argument);
	}
}

void lone_vector_push_all(struct lone_lisp *lone, struct lone_value vector, size_t count, ...)
{
	va_list arguments;

	va_start(arguments, count);
	lone_vector_push_va_list(lone, vector, count, arguments);
	va_end(arguments);
}

struct lone_value lone_vector_build(struct lone_lisp *lone, size_t count, ...)
{
	struct lone_value vector = lone_vector_create(lone, count);
	va_list arguments;

	va_start(arguments, count);
	lone_vector_push_va_list(lone, vector, count, arguments);
	va_end(arguments);

	return vector;
}

bool lone_vector_contains(struct lone_value vector, struct lone_value value)
{
	struct lone_heap_value *actual;
	size_t i;

	actual = vector.as.heap_value;

	for (i = 0; i < actual->as.vector.count; ++i) {
		if (lone_is_equal(value, actual->as.vector.values[i])) {
			return true;
		}
	}

	return false;
}
