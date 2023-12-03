/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/modules.h>
#include <lone/modules/intrinsic/list.h>

#include <lone/lisp/evaluator.h>

#include <lone/value/primitive.h>
#include <lone/value/list.h>
#include <lone/value/table.h>
#include <lone/value/symbol.h>

#include <lone/linux.h>

void lone_modules_intrinsic_list_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_intern_c_string(lone, "list"),
	                  *module = lone_module_for_name(lone, name),
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false };

	primitive = lone_primitive_create(lone, "construct", lone_primitive_list_construct, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "construct"), primitive);

	primitive = lone_primitive_create(lone, "first", lone_primitive_list_first, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "first"), primitive);

	primitive = lone_primitive_create(lone, "rest", lone_primitive_list_rest, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "rest"), primitive);

	primitive = lone_primitive_create(lone, "map", lone_primitive_list_map, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "map"), primitive);

	primitive = lone_primitive_create(lone, "reduce", lone_primitive_list_reduce, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "reduce"), primitive);

	primitive = lone_primitive_create(lone, "flatten", lone_primitive_list_flatten, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "flatten"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}


LONE_PRIMITIVE(list_construct)
{
	struct lone_value *first, *rest;

	if (lone_is_nil(arguments)) { /* no arguments given: (construct) */ linux_exit(-1); }

	first = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(arguments)) { /* only one argument given: (construct first) */ linux_exit(-1); }

	rest = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* more than two arguments given: (construct first rest extra) */ linux_exit(-1); }

	return lone_list_create(lone, first, rest);
}

LONE_PRIMITIVE(list_first)
{
	struct lone_value *argument;
	if (lone_is_nil(arguments)) { linux_exit(-1); }
	argument = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(argument)) { linux_exit(-1); }
	if (!lone_is_nil(arguments)) { linux_exit(-1); }
	return lone_list_first(argument);
}

LONE_PRIMITIVE(list_rest)
{
	struct lone_value *argument;
	if (lone_is_nil(arguments)) { linux_exit(-1); }
	argument = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(argument)) { linux_exit(-1); }
	if (!lone_is_nil(arguments)) { linux_exit(-1); }
	return lone_list_rest(argument);
}

LONE_PRIMITIVE(list_map)
{
	struct lone_value *function, *list, *results, *head;

	if (lone_is_nil(arguments)) { /* arguments not given */ linux_exit(-1); }
	function = lone_list_first(arguments);
	if (!lone_is_applicable(function)) { /* not given an applicable value */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);
	list = lone_list_first(arguments);
	if (!lone_is_list(list)) { /* can only map functions to lists */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* too many arguments given */ linux_exit(-1); }

	results = lone_list_create_nil(lone);

	for (head = results; !lone_is_nil(list); list = lone_list_rest(list)) {
		arguments = lone_list_create(lone, lone_list_first(list), lone_nil(lone));
		head = lone_list_append(lone, head, lone_apply(lone, module, environment, function, arguments));
	}

	return results;
}

LONE_PRIMITIVE(list_reduce)
{
	struct lone_value *function, *list, *result;

	if (lone_is_nil(arguments)) { /* arguments not given */ linux_exit(-1); }
	function = lone_list_first(arguments);
	if (!lone_is_applicable(function)) { /* not given an applicable value */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);
	result = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	list = lone_list_first(arguments);
	if (!lone_is_list(list)) { /* can only map functions to lists */ linux_exit(-1); }
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* too many arguments given */ linux_exit(-1); }

	for (/* list */; !lone_is_nil(list); list = lone_list_rest(list)) {
		arguments = lone_list_build(lone, 2, result, lone_list_first(list));
		result = lone_apply(lone, module, environment, function, arguments);
	}

	return result;
}

LONE_PRIMITIVE(list_flatten)
{
	return lone_list_flatten(lone, arguments);
}
