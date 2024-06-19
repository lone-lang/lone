/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/value.h>
#include <lone/lisp/value/function.h>

#include <lone/lisp/heap.h>

struct lone_lisp_value lone_lisp_function_create(struct lone_lisp *lone,
		struct lone_lisp_value arguments, struct lone_lisp_value code,
		struct lone_lisp_value environment, struct lone_lisp_function_flags flags)
{
	struct lone_lisp_heap_value *actual = lone_lisp_heap_allocate_value(lone);
	actual->type = LONE_LISP_TYPE_FUNCTION;
	actual->as.function.arguments = arguments;
	actual->as.function.code = code;
	actual->as.function.environment = environment;
	actual->as.function.flags = flags;
	return lone_lisp_value_from_heap_value(actual);
}
