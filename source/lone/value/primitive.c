/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/value.h>
#include <lone/value/primitive.h>
#include <lone/value/symbol.h>
#include <lone/memory/heap.h>

struct lone_value lone_primitive_create(
	struct lone_lisp *lone,
	char *name,
	lone_primitive function,
	struct lone_value closure,
	struct lone_function_flags flags)
{
	struct lone_heap_value *actual = lone_heap_allocate_value(lone);
	actual->type = LONE_TYPE_PRIMITIVE;
	actual->as.primitive.name = lone_intern_c_string(lone, name);
	actual->as.primitive.function = function;
	actual->as.primitive.closure = closure;
	actual->as.primitive.flags = flags;
	return lone_value_from_heap_value(actual);
}

