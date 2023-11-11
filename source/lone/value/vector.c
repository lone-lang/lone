/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/linux.h>
#include <lone/value.h>
#include <lone/value/vector.h>
#include <lone/value/list.h>
#include <lone/memory/allocator.h>

#include <lone/struct/value.h>
#include <lone/struct/vector.h>

struct lone_value *lone_vector_create(struct lone_lisp *lone, size_t capacity)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_VECTOR;
	value->vector.capacity = capacity;
	value->vector.count = 0;
	value->vector.values = lone_allocate(lone, capacity * sizeof(*value->vector.values));
	for (size_t i = 0; i < value->vector.capacity; ++i) { value->vector.values[i] = 0; }
	return value;
}

void lone_vector_resize(struct lone_lisp *lone, struct lone_value *vector, size_t new_capacity)
{
	struct lone_value **new = lone_allocate(lone, new_capacity * sizeof(struct lone_value *));
	size_t i;

	for (i = 0; i < new_capacity; ++i) {
		new[i] = i < vector->vector.count? vector->vector.values[i] : 0;
	}

	lone_deallocate(lone, vector->vector.values);

	vector->vector.values = new;
	vector->vector.capacity = new_capacity;
	if (vector->vector.count > new_capacity) { vector->vector.count = new_capacity; }
}

struct lone_value *lone_vector_get_value_at(struct lone_lisp *lone, struct lone_value *vector, size_t i)
{
	struct lone_value *value = i < vector->vector.capacity? vector->vector.values[i] : lone_nil(lone);
	return value? value : lone_nil(lone);
}

struct lone_value *lone_vector_get(struct lone_lisp *lone, struct lone_value *vector, struct lone_value *index)
{
	if (!lone_is_integer(index)) { /* only integer indexes supported */ linux_exit(-1); }
	return lone_vector_get_value_at(lone, vector, (size_t) index->integer);
}

void lone_vector_set_value_at(struct lone_lisp *lone, struct lone_value *vector, size_t i, struct lone_value *value)
{
	if (i >= vector->vector.capacity) { lone_vector_resize(lone, vector, i * 2); }
	vector->vector.values[i] = value;
	if (++i > vector->vector.count) { vector->vector.count = i; }
}

void lone_vector_set(struct lone_lisp *lone, struct lone_value *vector, struct lone_value *index, struct lone_value *value)
{
	if (!lone_is_integer(index)) { /* only integer indexes supported */ linux_exit(-1); }
	lone_vector_set_value_at(lone, vector, (size_t) index->integer, value);
}

void lone_vector_push(struct lone_lisp *lone, struct lone_value *vector, struct lone_value *value)
{
	lone_vector_set_value_at(lone, vector, vector->vector.count, value);
}

void lone_vector_push_va_list(struct lone_lisp *lone, struct lone_value *vector, size_t count, va_list arguments)
{
	struct lone_value *argument;
	size_t i;

	for (i = 0; i < count; ++i) {
		argument = va_arg(arguments, struct lone_value *);
		lone_vector_push(lone, vector, argument);
	}
}

void lone_vector_push_all(struct lone_lisp *lone, struct lone_value *vector, size_t count, ...)
{
	va_list arguments;

	va_start(arguments, count);
	lone_vector_push_va_list(lone, vector, count, arguments);
	va_end(arguments);
}

struct lone_value *lone_vector_build(struct lone_lisp *lone, size_t count, ...)
{
	struct lone_value *vector = lone_vector_create(lone, count);
	va_list arguments;

	va_start(arguments, count);
	lone_vector_push_va_list(lone, vector, count, arguments);
	va_end(arguments);

	return vector;
}

bool lone_vector_contains(struct lone_value *vector, struct lone_value *value)
{
	size_t i;

	for (i = 0; i < vector->vector.count; ++i) {
		if (lone_is_equal(value, vector->vector.values[i])) {
			return true;
		}
	}

	return false;
}
