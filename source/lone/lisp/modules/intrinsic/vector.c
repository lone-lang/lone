/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/vector.h>
#include <lone/lisp/modules/intrinsic/lone.h>

#include <lone/lisp/machine.h>
#include <lone/lisp/machine/stack.h>
#include <lone/lisp/module.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_vector_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_value name, module;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "vector");
	module = lone_lisp_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	lone_lisp_module_export_primitive(lone, module, "get",
			"vector_get", lone_lisp_primitive_vector_get, module, flags);

	lone_lisp_module_export_primitive(lone, module, "set",
			"vector_set", lone_lisp_primitive_vector_set, module, flags);

	lone_lisp_module_export_primitive(lone, module, "slice",
			"vector_slice", lone_lisp_primitive_vector_slice, module, flags);

	lone_lisp_module_export_primitive(lone, module, "each",
			"vector_each", lone_lisp_primitive_vector_each, module, flags);

	lone_lisp_module_export_primitive(lone, module, "count",
			"vector_count", lone_lisp_primitive_vector_count, module, flags);
}

LONE_LISP_PRIMITIVE(vector_get)
{
	struct lone_lisp_value arguments, vector, index, value;

	switch (step) {
	case 0:

		arguments = lone_lisp_machine_pop_value(lone, machine);

		goto destructure;

	case 1: /* resumed with replacement argument list */

		arguments = machine->value;

		if (!lone_lisp_is_list(lone, arguments)) {
			/* cannot destructure */
			return
				lone_lisp_signal_emit(
					lone,
					machine,
					1,
					lone->symbols.tags.type_error,
					arguments
				);
		}

		goto destructure;

	case 2: /* resumed with replacement vector from type-error */

		index  = lone_lisp_machine_pop_value(lone, machine);
		vector = machine->value;

		goto check_vector;

	case 3: /* resumed with replacement index from type-error */

		vector = lone_lisp_machine_pop_value(lone, machine);
		index  = machine->value;

		goto check_index;

	default:
		__builtin_trap();
	}

destructure:

	if (lone_lisp_list_destructure(lone, arguments, 2, &vector, &index)) {
		/* wrong number of arguments: (get), (get [] 0 "extra") */
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				lone->symbols.tags.arity_error,
				arguments
			);
	}

check_vector:

	if (!lone_lisp_is_vector(lone, vector)) {
		/* vector not given: (get {}) */
		lone_lisp_machine_push_value(lone, machine, index);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				2,
				lone->symbols.tags.type_error,
				vector
			);
	}

check_index:

	if (!lone_lisp_is_integer(lone, index)) {
		/* integer index not given: (get [1 2 3] "invalid") */
		lone_lisp_machine_push_value(lone, machine, vector);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				3,
				lone->symbols.tags.type_error,
				index
			);
	}

	value = lone_lisp_vector_get(lone, vector, index);

	lone_lisp_machine_push_value(lone, machine, value);
	return 0;
}

LONE_LISP_PRIMITIVE(vector_set)
{
	struct lone_lisp_value arguments, vector, index, value;

	switch (step) {
	case 0:

		arguments = lone_lisp_machine_pop_value(lone, machine);

		goto destructure;

	case 1: /* resumed with replacement argument list */

		arguments = machine->value;

		if (!lone_lisp_is_list(lone, arguments)) {
			/* cannot destructure */
			return
				lone_lisp_signal_emit(
					lone,
					machine,
					1,
					lone->symbols.tags.type_error,
					arguments
				);
		}

		goto destructure;

	case 2: /* resumed with replacement vector from type-error */

		value  = lone_lisp_machine_pop_value(lone, machine);
		index  = lone_lisp_machine_pop_value(lone, machine);
		vector = machine->value;

		goto check_vector;

	case 3: /* resumed with replacement index from type-error */

		value  = lone_lisp_machine_pop_value(lone, machine);
		vector = lone_lisp_machine_pop_value(lone, machine);
		index  = machine->value;

		goto check_index;

	default:
		__builtin_trap();
	}

destructure:

	if (lone_lisp_list_destructure(lone, arguments, 3, &vector, &index, &value)) {
		/* wrong number of arguments: (set), (set [] 0 1 "extra") */
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				lone->symbols.tags.arity_error,
				arguments
			);
	}

check_vector:

	if (!lone_lisp_is_vector(lone, vector)) {
		/* vector not given: (set {}) */
		lone_lisp_machine_push_value(lone, machine, index);
		lone_lisp_machine_push_value(lone, machine, value);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				2,
				lone->symbols.tags.type_error,
				vector
			);
	}

check_index:

	if (!lone_lisp_is_integer(lone, index)) {
		/* integer index not given: (set [1 2 3] "invalid") */
		lone_lisp_machine_push_value(lone, machine, vector);
		lone_lisp_machine_push_value(lone, machine, value);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				3,
				lone->symbols.tags.type_error,
				index
			);
	}

	lone_lisp_vector_set(lone, vector, index, value);

	lone_lisp_machine_push_value(lone, machine, value);
	return 0;
}

LONE_LISP_PRIMITIVE(vector_slice)
{
	struct lone_lisp_value arguments, vector, start, end, slice;
	size_t i, j, k, count;
	lone_lisp_integer index;
	bool has_end;

	switch (step) {
	case 0:

		arguments = lone_lisp_machine_pop_value(lone, machine);

		goto parse_arguments;

	case 1: /* resumed with replacement argument list */

		arguments = machine->value;

		if (!lone_lisp_list_is_proper(lone, arguments)) {
			/* cannot parse */
			return
				lone_lisp_signal_emit(
					lone,
					machine,
					1,
					lone->symbols.tags.type_error,
					arguments
				);
		}

		goto parse_arguments;

	case 2: /* resumed with replacement vector from type-error */

		has_end = lone_lisp_machine_pop_integer(lone, machine);
		end     = lone_lisp_machine_pop_value(lone, machine);
		start   = lone_lisp_machine_pop_value(lone, machine);
		vector  = machine->value;

		goto validate_vector;

	case 3: /* resumed with replacement start from type-error or index-error */

		has_end = lone_lisp_machine_pop_integer(lone, machine);
		end     = lone_lisp_machine_pop_value(lone, machine);
		vector  = lone_lisp_machine_pop_value(lone, machine);
		start   = machine->value;

		goto validate_start;

	case 4: /* resumed with replacement end from type-error or index-error */

		start   = lone_lisp_machine_pop_value(lone, machine);
		vector  = lone_lisp_machine_pop_value(lone, machine);
		end     = machine->value;
		has_end = true;

		goto validate_end;

	default:
		__builtin_trap();
	}

parse_arguments:

	if (lone_lisp_is_nil(arguments)) {
		/* arguments not given: (slice) */
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				lone->symbols.tags.arity_error,
				arguments
			);
	}

	vector = lone_lisp_list_first(lone, arguments);
	arguments = lone_lisp_list_rest(lone, arguments);

	if (lone_lisp_is_nil(arguments)) {
		/* start index not given: (slice vector) */
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				lone->symbols.tags.arity_error,
				arguments
			);
	}

	start = lone_lisp_list_first(lone, arguments);
	arguments = lone_lisp_list_rest(lone, arguments);

	has_end = !lone_lisp_is_nil(arguments);

	if (has_end) {
		end = lone_lisp_list_first(lone, arguments);
		arguments = lone_lisp_list_rest(lone, arguments);

		if (!lone_lisp_is_nil(arguments)) {
			/* too many arguments given: (slice vector start end extra) */
			return
				lone_lisp_signal_emit(
					lone,
					machine,
					1,
					lone->symbols.tags.arity_error,
					arguments
				);
		}
	} else {
		end = lone_lisp_nil();
	}

validate_vector:

	if (!lone_lisp_is_vector(lone, vector)) {
		/* vector not given: (slice {}) */
		lone_lisp_machine_push_value(lone, machine, start);
		lone_lisp_machine_push_value(lone, machine, end);
		lone_lisp_machine_push_integer(lone, machine, has_end);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				2,
				lone->symbols.tags.type_error,
				vector
			);
	}

validate_start:

	if (!lone_lisp_is_integer(lone, start)) {
		/* start is not an integer: (slice vector "error") */
		lone_lisp_machine_push_value(lone, machine, vector);
		lone_lisp_machine_push_value(lone, machine, end);
		lone_lisp_machine_push_integer(lone, machine, has_end);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				3,
				lone->symbols.tags.type_error,
				start
			);
	}

	index = lone_lisp_integer_of(start);
	count = lone_lisp_vector_count(lone, vector);

	if (index < 0) {
		/* negative indexes not supported: (slice vector -10) */
		lone_lisp_machine_push_value(lone, machine, vector);
		lone_lisp_machine_push_value(lone, machine, end);
		lone_lisp_machine_push_integer(lone, machine, has_end);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				3,
				lone->symbols.tags.index_error,
				start
			);
	}

	if ((size_t) index > count) {
		/* start past end of vector: (slice [] 1 2) */
		lone_lisp_machine_push_value(lone, machine, vector);
		lone_lisp_machine_push_value(lone, machine, end);
		lone_lisp_machine_push_integer(lone, machine, has_end);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				3,
				lone->symbols.tags.index_error,
				start
			);
	}

	i = (size_t) index;

	if (!has_end) {
		j = count;
		goto build_slice;
	}

validate_end:

	/* case 4 resumes here with uninitialized index, count and i
	   derive them from the already validated start and vector   */
	index = lone_lisp_integer_of(start);
	count = lone_lisp_vector_count(lone, vector);
	i = (size_t) index;

	if (!lone_lisp_is_integer(lone, end)) {
		/* end is not an integer: (slice vector 10 "error") */
		lone_lisp_machine_push_value(lone, machine, vector);
		lone_lisp_machine_push_value(lone, machine, start);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				4,
				lone->symbols.tags.type_error,
				end
			);
	}

	index = lone_lisp_integer_of(end);

	if (index < 0) {
		/* negative indexes not supported: (slice vector 0 -10) */
		lone_lisp_machine_push_value(lone, machine, vector);
		lone_lisp_machine_push_value(lone, machine, start);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				4,
				lone->symbols.tags.index_error,
				end
			);
	}

	if ((size_t) index > count) {
		/* end past end of vector */
		lone_lisp_machine_push_value(lone, machine, vector);
		lone_lisp_machine_push_value(lone, machine, start);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				4,
				lone->symbols.tags.index_error,
				end
			);
	}

	j = (size_t) index;

	if (j < i) {
		/* end before start: (slice vector 10 5) */
		lone_lisp_machine_push_value(lone, machine, vector);
		lone_lisp_machine_push_value(lone, machine, start);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				4,
				lone->symbols.tags.index_error,
				end
			);
	}

build_slice:

	slice = lone_lisp_vector_create(lone, j - i);

	for (k = 0; i < j; ++i, ++k) {
		lone_lisp_vector_set_value_at(lone, slice, k, lone_lisp_vector_get_value_at(lone, vector, i));
	}

	lone_lisp_machine_push_value(lone, machine, slice);
	return 0;
}

LONE_LISP_PRIMITIVE(vector_each)
{
	struct lone_lisp_value arguments, vector, function, entry;
	lone_lisp_integer i;

	switch (step) {
	case 0:
		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(lone, arguments, 2, &vector, &function)) {
			/* wrong number of arguments */ linux_exit(-1);
		}

		if (!lone_lisp_is_vector(lone, vector)) { /* vector not given: (each {}) */ linux_exit(-1); }
		if (!lone_lisp_is_applicable(lone, function)) {
			/* applicable not given: (each vector []) */ linux_exit(-1);
		}

		if (lone_lisp_vector_count(lone, vector) < 1) {
			/* nothing to do */ break;
		}

		i = 0;

	iteration:

		entry = lone_lisp_vector_get_value_at(lone, vector, i);

		machine->applicable = function;
		machine->list = lone_lisp_list_build(lone, 1, &entry);
		machine->step = LONE_LISP_MACHINE_STEP_APPLY;

		lone_lisp_machine_push_integer(lone, machine, i);
		lone_lisp_machine_push_value(lone, machine, function);
		lone_lisp_machine_push_value(lone, machine, vector);

		return 1;

	case 1: /* advance or finish iteration */

		vector   = lone_lisp_machine_pop_value(lone, machine);
		function = lone_lisp_machine_pop_value(lone, machine);
		i        = lone_lisp_machine_pop_integer(lone, machine);

		++i;

		if (i < lone_lisp_vector_count(lone, vector)) {
			goto iteration;
		} else {
			break;
		}

	default:
		linux_exit(-1);
	}

	lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
	return 0;
}

LONE_LISP_PRIMITIVE(vector_count)
{
	struct lone_lisp_value arguments, vector, count;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(lone, arguments, 1, &vector)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_vector(lone, vector)) { /* vector not given: (count {}) */ linux_exit(-1); }

	count = lone_lisp_integer_create(lone_lisp_vector_count(lone, vector));

	lone_lisp_machine_push_value(lone, machine, count);
	return 0;
}
