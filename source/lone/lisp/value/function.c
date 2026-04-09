/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>
#include <lone/lisp/heap.h>

struct lone_lisp_value lone_lisp_function_create(struct lone_lisp *lone,
		struct lone_lisp_value arguments, struct lone_lisp_value code,
		struct lone_lisp_value environment, struct lone_lisp_function_flags flags)
{
	struct lone_lisp_heap_value *actual = lone_lisp_heap_allocate_value(lone);
	struct lone_lisp_value value;

	actual->as.function.arguments = arguments;
	actual->as.function.code = code;
	actual->as.function.environment = environment;
	actual->as.function.flags = flags;
	value = lone_lisp_value_from_heap_value(lone, actual, LONE_LISP_TAG_FUNCTION);

	/* Encode FEXPR flags in the metadata field at bits 8-9.
	 * This allows should_evaluate_operands to check the flags
	 * with a single bit test on the tagged word, without
	 * dereferencing the heap to read the flags structure.
	 */
	if (flags.evaluate_arguments) { value.tagged |= LONE_LISP_METADATA_EVALUATE_ARGUMENTS; }
	if (flags.evaluate_result)    { value.tagged |= LONE_LISP_METADATA_EVALUATE_RESULT; }

	return value;
}
