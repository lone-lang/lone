/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/value.h>
#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/symbol.h>

#include <lone/lisp/heap.h>

struct lone_lisp_value lone_lisp_primitive_create(struct lone_lisp *lone,
		char *name, lone_lisp_primitive_function function,
		struct lone_lisp_value closure, struct lone_lisp_function_flags flags)
{
	struct lone_lisp_heap_value *actual = lone_lisp_heap_allocate_value(lone);
	actual->type = LONE_LISP_TYPE_PRIMITIVE;
	actual->as.primitive.name = lone_lisp_intern_c_string(lone, name);
	actual->as.primitive.function = function;
	actual->as.primitive.closure = closure;
	actual->as.primitive.flags = flags;
	return lone_lisp_value_from_heap_value(actual);
}
