/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>
#include <lone/lisp/heap.h>

static unsigned long lone_lisp_count_arguments(struct lone_lisp *lone,
		struct lone_lisp_value arguments)
{
	unsigned long count = 0;

	while (!lone_lisp_is_nil(arguments)) {
		++count;

		if (lone_lisp_is_list(lone, lone_lisp_list_first(lone, arguments))) {
			/* variadic parameter: ((rest))
			   counts as one, terminates formal arguments list */
			break;
		}

		arguments = lone_lisp_list_rest(lone, arguments);
	}

	return count;
}

struct lone_lisp_value lone_lisp_function_create(struct lone_lisp *lone,
		struct lone_lisp_value arguments, struct lone_lisp_value code,
		struct lone_lisp_value environment, struct lone_lisp_function_flags flags)
{
	struct lone_lisp_heap_value *actual = lone_lisp_heap_allocate_value(lone);
	struct lone_lisp_value value;
	unsigned long arity;

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

	/* Encode function arity in metadata bits 10-13.
	 * This allows determining the arity of most functions
	 * without any heap access.
	 */
	arity = lone_lisp_count_arguments(lone, arguments);
	if (arity > LONE_LISP_METADATA_ARITY_OVERFLOW) {
		arity = LONE_LISP_METADATA_ARITY_OVERFLOW;
	}
	value.tagged |= (long) arity << LONE_LISP_METADATA_ARITY_SHIFT;

	return value;
}
