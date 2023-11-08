#include <lone/value.h>
#include <lone/value/vector.h>

#include <lone/struct/value.h>
#include <lone/struct/vector.h>

#include <lone/memory/allocator.h>

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
