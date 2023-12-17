/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/linux.h>
#include <lone/value.h>
#include <lone/value/vector.h>
#include <lone/value/list.h>
#include <lone/memory/heap.h>
#include <lone/memory/array.h>

struct lone_value lone_vector_create(struct lone_lisp *lone, size_t capacity)
{
	struct lone_heap_value *heap_value = lone_heap_allocate_value(lone);
	struct lone_vector *actual = &heap_value->as.vector;

	heap_value->type = LONE_VECTOR;
	actual->count = 0;
	actual->capacity = capacity;
	actual->values = lone_memory_array(lone, 0, actual->capacity, sizeof(*actual->values));

	return lone_value_from_heap_value(heap_value);
}

size_t lone_vector_count(struct lone_value vector)
{
	return vector.as.heap_value->as.vector.count;
}

void lone_vector_resize(struct lone_lisp *lone, struct lone_value vector, size_t new_capacity)
{
	struct lone_vector *actual = &vector.as.heap_value->as.vector;

	actual->capacity = new_capacity;
	actual->values = lone_memory_array(lone, actual->values, actual->capacity, sizeof(*actual->values));

	if (actual->count > new_capacity) {
		/* vector has shrunk, truncate count */
		actual->count = new_capacity;
	}
}

struct lone_value lone_vector_get_value_at(struct lone_lisp *lone, struct lone_value vector, size_t i)
{
	struct lone_vector *actual;

	actual = &vector.as.heap_value->as.vector;

	if (lone_memory_array_is_bounded(i, actual->capacity, sizeof(*actual->values))) {
		return actual->values[i];
	} else {
		return lone_nil();
	}
}

struct lone_value lone_vector_get(struct lone_lisp *lone, struct lone_value vector, struct lone_value index)
{
	if (!lone_is_integer(index)) { /* only integer indexes supported */ linux_exit(-1); }
	return lone_vector_get_value_at(lone, vector, index.as.unsigned_integer);
}

void lone_vector_set_value_at(struct lone_lisp *lone, struct lone_value vector, size_t i, struct lone_value value)
{
	struct lone_vector *actual;

	actual = &vector.as.heap_value->as.vector;

	if (!lone_memory_array_is_bounded(i, actual->capacity, sizeof(*actual->values))) {
		lone_vector_resize(lone, vector, 2 * (i + 1));
	}

	actual->values[i] = value;
	if (++i > actual->count) { actual->count = i; }
}

void lone_vector_set(struct lone_lisp *lone, struct lone_value vector, struct lone_value index, struct lone_value value)
{
	if (!lone_is_integer(index)) { /* only integer indexes supported */ linux_exit(-1); }
	lone_vector_set_value_at(lone, vector, index.as.unsigned_integer, value);
}

void lone_vector_push(struct lone_lisp *lone, struct lone_value vector, struct lone_value value)
{
	lone_vector_set_value_at(lone, vector, lone_vector_count(vector), value);
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
	struct lone_vector *actual;
	size_t i;

	actual = &vector.as.heap_value->as.vector;

	for (i = 0; i < actual->count; ++i) {
		if (lone_is_equal(value, actual->values[i])) {
			return true;
		}
	}

	return false;
}
