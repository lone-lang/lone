/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/list.h>

#include <lone/lisp/module.h>
#include <lone/lisp/evaluator.h>

#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/symbol.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_list_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_value name, module, primitive;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "list");
	module = lone_lisp_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	primitive = lone_lisp_primitive_create(lone, "construct", lone_lisp_primitive_list_construct, module, flags);
	lone_lisp_module_set_and_export(lone, module, lone_lisp_intern_c_string(lone, "construct"), primitive);

	primitive = lone_lisp_primitive_create(lone, "first", lone_lisp_primitive_list_first, module, flags);
	lone_lisp_module_set_and_export(lone, module, lone_lisp_intern_c_string(lone, "first"), primitive);

	primitive = lone_lisp_primitive_create(lone, "rest", lone_lisp_primitive_list_rest, module, flags);
	lone_lisp_module_set_and_export(lone, module, lone_lisp_intern_c_string(lone, "rest"), primitive);

	primitive = lone_lisp_primitive_create(lone, "map", lone_lisp_primitive_list_map, module, flags);
	lone_lisp_module_set_and_export(lone, module, lone_lisp_intern_c_string(lone, "map"), primitive);

	primitive = lone_lisp_primitive_create(lone, "reduce", lone_lisp_primitive_list_reduce, module, flags);
	lone_lisp_module_set_and_export(lone, module, lone_lisp_intern_c_string(lone, "reduce"), primitive);

	primitive = lone_lisp_primitive_create(lone, "flatten", lone_lisp_primitive_list_flatten, module, flags);
	lone_lisp_module_set_and_export(lone, module, lone_lisp_intern_c_string(lone, "flatten"), primitive);

	lone_lisp_table_set(lone, lone->modules.loaded, name, module);
}


LONE_LISP_PRIMITIVE(list_construct)
{
	struct lone_lisp_value first, rest;

	if (lone_lisp_list_destructure(arguments, 2, &first, &rest)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	return lone_lisp_list_create(lone, first, rest);
}

LONE_LISP_PRIMITIVE(list_first)
{
	struct lone_lisp_value argument;

	if (lone_lisp_list_destructure(arguments, 1, &argument)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	return lone_lisp_list_first(argument);
}

LONE_LISP_PRIMITIVE(list_rest)
{
	struct lone_lisp_value argument;

	if (lone_lisp_list_destructure(arguments, 1, &argument)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	return lone_lisp_list_rest(argument);
}

LONE_LISP_PRIMITIVE(list_map)
{
	struct lone_lisp_value function, list, results, head;

	if (lone_lisp_list_destructure(arguments, 2, &function, &list)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_applicable(function)) { /* not given an applicable value */ linux_exit(-1); }
	if (lone_lisp_is_nil(list)) { /* mapping function to empty list */ return lone_lisp_nil(); }
	if (!lone_lisp_is_list(list)) { /* can only map functions to lists */ linux_exit(-1); }

	for (results = head = lone_lisp_nil(); !lone_lisp_is_nil(list); list = lone_lisp_list_rest(list)) {
		arguments = lone_lisp_list_create(lone, lone_lisp_list_first(list), lone_lisp_nil());
		lone_lisp_list_append(lone, &results, &head,
				lone_lisp_apply(lone, module, environment, function, arguments));
	}

	return results;
}

LONE_LISP_PRIMITIVE(list_reduce)
{
	struct lone_lisp_value function, initial, list, result, head, current;

	if (lone_lisp_list_destructure(arguments, 3, &function, &initial, &list)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_applicable(function)) { /* not given an applicable value */ linux_exit(-1); }
	if (lone_lisp_is_nil(list)) { /* mapping function to empty list */ return initial; }
	if (!lone_lisp_is_list(list)) { /* can only reduce lists */ linux_exit(-1); }

	for (result = initial, head = list; !lone_lisp_is_nil(head); head = lone_lisp_list_rest(head)) {
		current = lone_lisp_list_first(head);
		arguments = lone_lisp_list_build(lone, 2, &result, &current);
		result = lone_lisp_apply(lone, module, environment, function, arguments);
	}

	return result;
}

LONE_LISP_PRIMITIVE(list_flatten)
{
	return lone_lisp_list_flatten(lone, arguments);
}
