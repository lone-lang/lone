/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/list.h>

#include <lone/lisp/machine.h>
#include <lone/lisp/module.h>

#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/symbol.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_list_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_value name, module;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "list");
	module = lone_lisp_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	lone_lisp_module_export_primitive(lone, module, "construct",
			"construct", lone_lisp_primitive_list_construct, module, flags);

	lone_lisp_module_export_primitive(lone, module, "first",
			"first", lone_lisp_primitive_list_first, module, flags);

	lone_lisp_module_export_primitive(lone, module, "rest",
			"rest", lone_lisp_primitive_list_rest, module, flags);

	lone_lisp_module_export_primitive(lone, module, "map",
			"map", lone_lisp_primitive_list_map, module, flags);

	lone_lisp_module_export_primitive(lone, module, "reduce",
			"reduce", lone_lisp_primitive_list_reduce, module, flags);

	lone_lisp_module_export_primitive(lone, module, "flatten",
			"flatten", lone_lisp_primitive_list_flatten, module, flags);
}


LONE_LISP_PRIMITIVE(list_construct)
{
	struct lone_lisp_value arguments, first, rest;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(arguments, 2, &first, &rest)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	lone_lisp_machine_push_value(lone, machine, lone_lisp_list_create(lone, first, rest));

	return 0;
}

LONE_LISP_PRIMITIVE(list_first)
{
	struct lone_lisp_value arguments, argument;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(arguments, 1, &argument)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	lone_lisp_machine_push_value(lone, machine, lone_lisp_list_first(argument));

	return 0;
}

LONE_LISP_PRIMITIVE(list_rest)
{
	struct lone_lisp_value arguments, argument;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(arguments, 1, &argument)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	lone_lisp_machine_push_value(lone, machine, lone_lisp_list_rest(argument));

	return 0;
}

LONE_LISP_PRIMITIVE(list_flatten)
{
	struct lone_lisp_value arguments;

	arguments = lone_lisp_machine_pop_value(lone, machine);
	lone_lisp_machine_push_value(lone, machine, lone_lisp_list_flatten(lone, arguments));

	return 0;
}

LONE_LISP_PRIMITIVE(list_map)
{
	struct lone_lisp_value arguments, function, list, head, results, result, entry, expression;

	switch (step) {
	case 0: /* unpack arguments and initialize */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(arguments, 2, &function, &list)) {
			/* wrong number of arguments */ linux_exit(-1);
		}

		if (!lone_lisp_is_applicable(function)) { /* not given an applicable value */ linux_exit(-1); }

		if (lone_lisp_is_nil(list)) {
			/* mapping function to empty list */
			lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
			return 0;
		}

		if (!lone_lisp_is_list(list)) { /* can only map functions to lists */ linux_exit(-1); }

		results = head = lone_lisp_nil();

	application: /* apply value to function */

		entry = lone_lisp_list_first(list);
		expression = lone_lisp_list_build(lone, 2, &function, &entry);

		lone->machine.step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		lone->machine.expression = expression;

		lone_lisp_machine_push_value(lone, machine, function);
		lone_lisp_machine_push_value(lone, machine, list);
		lone_lisp_machine_push_value(lone, machine, head);
		lone_lisp_machine_push_value(lone, machine, results);

		return 1;

	case 1: /* collect resulting value into results list */

		result   = machine->value;
		results  = lone_lisp_machine_pop_value(lone, machine);
		head     = lone_lisp_machine_pop_value(lone, machine);
		list     = lone_lisp_machine_pop_value(lone, machine);
		function = lone_lisp_machine_pop_value(lone, machine);

		lone_lisp_list_append(lone, &results, &head, result);
		list = lone_lisp_list_rest(list);

		if (!lone_lisp_is_nil(list)) {
			goto application;
		} else {
			lone_lisp_machine_push_value(lone, machine, results);
			return 0;
		}

	default:
		break;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(list_reduce)
{
	struct lone_lisp_value arguments, function, list, accumulator, entry;

	switch (step) {
	case 0: /* unpack and check arguments */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(arguments, 3, &function, &accumulator, &list)) {
			/* wrong number of arguments */ linux_exit(-1);
		}

		if (!lone_lisp_is_applicable(function)) { /* not given an applicable value */ linux_exit(-1); }

		if (lone_lisp_is_nil(list)) {
			/* mapping function to empty list */
			lone_lisp_machine_push_value(lone, machine, accumulator);
			return 0;
		}

		if (!lone_lisp_is_list(list)) { /* can only reduce lists */ linux_exit(-1); }

	application: /* apply accumulator and value to function */

		entry = lone_lisp_list_first(list);

		lone->machine.step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		lone->machine.expression = lone_lisp_list_build(lone, 3, &function, &accumulator, &entry);

		lone_lisp_machine_push_value(lone, machine, function);
		lone_lisp_machine_push_value(lone, machine, list);
		/* no need to push the accumulator because the result of the
		 * function application will be its new value */

		return 1;

	case 1: /* collect result of function application */

		accumulator = machine->value;
		list        = lone_lisp_machine_pop_value(lone, machine);
		function    = lone_lisp_machine_pop_value(lone, machine);

		list = lone_lisp_list_rest(list);

		if (!lone_lisp_is_nil(list)) {
			goto application;
		} else {
			lone_lisp_machine_push_value(lone, machine, accumulator);
			return 0;
		}
	default:
		break;
	}

	linux_exit(-1);
}
