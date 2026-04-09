/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>
#include <lone/lisp/heap.h>

struct lone_lisp_value lone_lisp_primitive_create(struct lone_lisp *lone,
		char *name, lone_lisp_primitive_function function,
		struct lone_lisp_value closure, struct lone_lisp_function_flags flags)
{
	struct lone_lisp_heap_value *actual = lone_lisp_heap_allocate_value(lone);
	struct lone_lisp_value value;

	actual->as.primitive.name = lone_lisp_intern_c_string(lone, name);
	actual->as.primitive.function = function;
	actual->as.primitive.closure = closure;
	actual->as.primitive.flags = flags;
	value = lone_lisp_value_from_heap_value(lone, actual, LONE_LISP_TAG_PRIMITIVE);

	/* Encode FEXPR flags in the metadata field at bits 8-9,
	 * same layout as functions.
	 */
	if (flags.evaluate_arguments) { value.tagged |= LONE_LISP_METADATA_EVALUATE_ARGUMENTS; }
	if (flags.evaluate_result)    { value.tagged |= LONE_LISP_METADATA_EVALUATE_RESULT; }

	return value;
}
