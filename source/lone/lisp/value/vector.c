/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/value.h>
#include <lone/lisp/value/vector.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/heap.h>

#include <lone/memory/array.h>

#include <lone/linux.h>

struct lone_lisp_value lone_lisp_vector_create(struct lone_lisp *lone, size_t capacity)
{
	struct lone_lisp_heap_value *heap_value = lone_lisp_heap_allocate_value(lone);
	struct lone_lisp_vector *actual = &heap_value->as.vector;

	heap_value->type = LONE_LISP_TYPE_VECTOR;
	actual->count = 0;
	actual->capacity = capacity;
	actual->values = lone_memory_array(lone->system, 0, actual->capacity, sizeof(*actual->values));

	return lone_lisp_value_from_heap_value(heap_value);
}

size_t lone_lisp_vector_count(struct lone_lisp_value vector)
{
	return lone_lisp_heap_value_of(vector)->as.vector.count;
}

void lone_lisp_vector_resize(struct lone_lisp *lone, struct lone_lisp_value vector, size_t new_capacity)
{
	struct lone_lisp_vector *actual = &lone_lisp_heap_value_of(vector)->as.vector;

	actual->capacity = new_capacity;
	actual->values = lone_memory_array(lone->system, actual->values, actual->capacity, sizeof(*actual->values));

	if (actual->count > new_capacity) {
		/* vector has shrunk, truncate count */
		actual->count = new_capacity;
	}
}

struct lone_lisp_value lone_lisp_vector_get_value_at(struct lone_lisp_value vector, size_t i)
{
	struct lone_lisp_vector *actual;

	actual = &lone_lisp_heap_value_of(vector)->as.vector;

	if (lone_memory_array_is_bounded(i, actual->capacity, sizeof(*actual->values))) {
		return actual->values[i];
	} else {
		return lone_lisp_nil();
	}
}

struct lone_lisp_value lone_lisp_vector_get(struct lone_lisp *lone,
		struct lone_lisp_value vector, struct lone_lisp_value index)
{
	if (!lone_lisp_is_integer(index)) { /* only integer indexes supported */ linux_exit(-1); }
	return lone_lisp_vector_get_value_at(vector, lone_lisp_integer_of(index));
}

void lone_lisp_vector_set_value_at(struct lone_lisp *lone, struct lone_lisp_value vector,
		size_t i, struct lone_lisp_value value)
{
	struct lone_lisp_vector *actual;

	actual = &lone_lisp_heap_value_of(vector)->as.vector;

	if (!lone_memory_array_is_bounded(i, actual->capacity, sizeof(*actual->values))) {
		lone_lisp_vector_resize(lone, vector, 2 * (i + 1));
	}

	actual->values[i] = value;
	if (++i > actual->count) { actual->count = i; }
}

void lone_lisp_vector_set(struct lone_lisp *lone, struct lone_lisp_value vector,
		struct lone_lisp_value index, struct lone_lisp_value value)
{
	if (!lone_lisp_is_integer(index)) { /* only integer indexes supported */ linux_exit(-1); }
	lone_lisp_vector_set_value_at(lone, vector, lone_lisp_integer_of(index), value);
}

void lone_lisp_vector_push(struct lone_lisp *lone,
		struct lone_lisp_value vector, struct lone_lisp_value value)
{
	lone_lisp_vector_set_value_at(lone, vector, lone_lisp_vector_count(vector), value);
}

void lone_lisp_vector_push_va_list(struct lone_lisp *lone,
		struct lone_lisp_value vector, size_t count, va_list arguments)
{
	struct lone_lisp_value *argument;
	size_t i;

	for (i = 0; i < count; ++i) {
		argument = va_arg(arguments, struct lone_lisp_value *);
		lone_lisp_vector_push(lone, vector, *argument);
	}
}

void lone_lisp_vector_push_all(struct lone_lisp *lone,
		struct lone_lisp_value vector, size_t count, ...)
{
	va_list arguments;

	va_start(arguments, count);
	lone_lisp_vector_push_va_list(lone, vector, count, arguments);
	va_end(arguments);
}

struct lone_lisp_value lone_lisp_vector_build(struct lone_lisp *lone, size_t count, ...)
{
	struct lone_lisp_value vector = lone_lisp_vector_create(lone, count);
	va_list arguments;

	va_start(arguments, count);
	lone_lisp_vector_push_va_list(lone, vector, count, arguments);
	va_end(arguments);

	return vector;
}

bool lone_lisp_vector_contains(struct lone_lisp_value vector, struct lone_lisp_value value)
{
	struct lone_lisp_value element;
	size_t i;

	LONE_LISP_VECTOR_FOR_EACH(element, vector, i) {
		if (lone_lisp_is_equal(value, element)) {
			return true;
		}
	}

	return false;
}
