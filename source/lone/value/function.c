/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/value.h>
#include <lone/value/function.h>
#include <lone/memory/heap.h>

struct lone_value lone_function_create(
	struct lone_lisp *lone,
	struct lone_value arguments,
	struct lone_value code,
	struct lone_value environment,
	struct lone_function_flags flags)
{
	struct lone_heap_value *actual = lone_heap_allocate_value(lone);
	actual->type = LONE_FUNCTION;
	actual->as.function.arguments = arguments;
	actual->as.function.code = code;
	actual->as.function.environment = environment;
	actual->as.function.flags = flags;
	return lone_value_from_heap_value(actual);
}
