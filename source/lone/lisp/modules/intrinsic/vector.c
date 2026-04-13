/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/vector.h>

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

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(lone, arguments, 2, &vector, &index)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_vector(lone, vector)) { /* vector not given: (get {}) */ linux_exit(-1); }
	if (!lone_lisp_is_integer(lone, index)) { /* integer index not given: (get [1 2 3] "invalid") */ linux_exit(-1); }

	value = lone_lisp_vector_get(lone, vector, index);

	lone_lisp_machine_push_value(lone, machine, value);
	return 0;
}

LONE_LISP_PRIMITIVE(vector_set)
{
	struct lone_lisp_value arguments, vector, index, value;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(lone, arguments, 3, &vector, &index, &value)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_vector(lone, vector)) { /* vector not given: (set {}) */ linux_exit(-1); }
	if (!lone_lisp_is_integer(lone, index)) { /* integer index not given: (set [1 2 3] "invalid") */ linux_exit(-1); }

	lone_lisp_vector_set(lone, vector, index, value);

	lone_lisp_machine_push_value(lone, machine, value);
	return 0;
}

LONE_LISP_PRIMITIVE(vector_slice)
{
	struct lone_lisp_value arguments, vector, start, end, slice;
	size_t i, j, k;
	lone_lisp_integer index;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_is_nil(arguments)) { /* arguments not given: (slice) */ linux_exit(-1); }

	vector = lone_lisp_list_first(lone, arguments);
	arguments = lone_lisp_list_rest(lone, arguments);
	if (!lone_lisp_is_vector(lone, vector)) { /* vector not given: (slice {}) */ linux_exit(-1); }
	if (lone_lisp_is_nil(arguments)) { /* start index not given: (slice vector) */ linux_exit(-1); }

	start = lone_lisp_list_first(lone, arguments);
	arguments = lone_lisp_list_rest(lone, arguments);
	if (!lone_lisp_is_integer(lone, start)) { /* start is not an integer: (slice vector "error") */ linux_exit(-1); }

	index = lone_lisp_integer_of(start);
	if (index < 0) { /* negative indexes not supported: (slice vector -10) */ linux_exit(-1); }
	i = (size_t) index;

	if (lone_lisp_is_nil(arguments)) {
		j = lone_lisp_vector_count(lone, vector);
	} else {
		end = lone_lisp_list_first(lone, arguments);
		arguments = lone_lisp_list_rest(lone, arguments);

		if (!lone_lisp_is_nil(arguments)) {
			/* too many arguments given: (slice vector start end extra) */ linux_exit(-1);
		}

		if (!lone_lisp_is_integer(lone, end)) {
			/* end is not an integer: (slice vector 10 "error") */ linux_exit(-1);
		}

		index = lone_lisp_integer_of(end);
		if (index < 0) { /* negative indexes not supported: (slice vector 0 -10) */ linux_exit(-1); }
		j = (size_t) index;
	}

	if (j < i) { /* end before start: (slice vector 10 5) */ linux_exit(-1); }

	slice = lone_lisp_vector_create(lone, j - i);

	for (k = 0; i < j; ++i, ++k) {
		lone_lisp_vector_set_value_at(lone, slice, k, lone_lisp_vector_get_value_at(lone, vector, i));
	}

	lone_lisp_machine_push_value(lone, machine, slice);
	return 0;
}

LONE_LISP_PRIMITIVE(vector_each)
{
	struct lone_lisp_value arguments, vector, function, entry, expression;
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
		expression = lone_lisp_list_build(lone, 2, &function, &entry);

		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		machine->expression = expression;

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
