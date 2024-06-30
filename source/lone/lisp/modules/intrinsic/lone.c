/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/lone.h>

#include <lone/lisp/module.h>
#include <lone/lisp/evaluator.h>
#include <lone/lisp/printer.h>
#include <lone/lisp/constants.h>
#include <lone/lisp/utilities.h>

#include <lone/lisp/value/function.h>
#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/symbol.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_lone_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_value name, module, primitive;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "lone");
	module = lone_lisp_module_for_name(lone, name);
	flags.evaluate_arguments = false;
	flags.evaluate_result = false;

	primitive = lone_lisp_primitive_create(lone, "begin", lone_lisp_primitive_lone_begin, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "begin", primitive);

	primitive = lone_lisp_primitive_create(lone, "when", lone_lisp_primitive_lone_when, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "when", primitive);

	primitive = lone_lisp_primitive_create(lone, "unless", lone_lisp_primitive_lone_unless, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "unless", primitive);

	primitive = lone_lisp_primitive_create(lone, "if", lone_lisp_primitive_lone_if, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "if", primitive);

	primitive = lone_lisp_primitive_create(lone, "let", lone_lisp_primitive_lone_let, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "let", primitive);

	primitive = lone_lisp_primitive_create(lone, "set", lone_lisp_primitive_lone_set, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "set", primitive);

	primitive = lone_lisp_primitive_create(lone, "quote", lone_lisp_primitive_lone_quote, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "quote", primitive);

	primitive = lone_lisp_primitive_create(lone, "quasiquote", lone_lisp_primitive_lone_quasiquote, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "quasiquote", primitive);

	primitive = lone_lisp_primitive_create(lone, "lambda", lone_lisp_primitive_lone_lambda, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "lambda", primitive);

	primitive = lone_lisp_primitive_create(lone, "lambda_bang", lone_lisp_primitive_lone_lambda_bang, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "lambda!", primitive);

	flags = (struct lone_lisp_function_flags) { .evaluate_arguments = true, .evaluate_result = false };

	primitive = lone_lisp_primitive_create(lone, "print", lone_lisp_primitive_lone_print, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "print", primitive);

	primitive = lone_lisp_primitive_create(lone, "is_list", lone_lisp_primitive_lone_is_list, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "list?", primitive);

	primitive = lone_lisp_primitive_create(lone, "is_vector", lone_lisp_primitive_lone_is_vector, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "vector?", primitive);

	primitive = lone_lisp_primitive_create(lone, "is_table", lone_lisp_primitive_lone_is_table, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "table?", primitive);

	primitive = lone_lisp_primitive_create(lone, "is_symbol", lone_lisp_primitive_lone_is_symbol, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "symbol?", primitive);

	primitive = lone_lisp_primitive_create(lone, "is_text", lone_lisp_primitive_lone_is_text, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "text?", primitive);

	primitive = lone_lisp_primitive_create(lone, "is_integer", lone_lisp_primitive_lone_is_integer, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "integer?", primitive);

	primitive = lone_lisp_primitive_create(lone, "is_identical", lone_lisp_primitive_lone_is_identical, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "identical?", primitive);

	primitive = lone_lisp_primitive_create(lone, "is_equivalent", lone_lisp_primitive_lone_is_equivalent, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "equivalent?", primitive);

	primitive = lone_lisp_primitive_create(lone, "is_equal", lone_lisp_primitive_lone_is_equal, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "equal?", primitive);

	lone_lisp_table_set(lone, lone->modules.loaded, name, module);
}


LONE_LISP_PRIMITIVE(lone_begin)
{
	struct lone_lisp_value value;

	for (value = lone_lisp_nil(); !lone_lisp_is_nil(arguments); arguments = lone_lisp_list_rest(arguments)) {
		value = lone_lisp_list_first(arguments);
		value = lone_lisp_evaluate(lone, module, environment, value);
	}

	return value;
}

LONE_LISP_PRIMITIVE(lone_when)
{
	struct lone_lisp_value test;

	if (lone_lisp_is_nil(arguments)) { /* test not specified: (when) */ linux_exit(-1); }
	test = lone_lisp_list_first(arguments);
	arguments = lone_lisp_list_rest(arguments);

	if (!lone_lisp_is_nil(lone_lisp_evaluate(lone, module, environment, test))) {
		return lone_lisp_primitive_lone_begin(lone, module, environment, arguments, closure);
	}

	return lone_lisp_nil();
}

LONE_LISP_PRIMITIVE(lone_unless)
{
	struct lone_lisp_value test;

	if (lone_lisp_is_nil(arguments)) { /* test not specified: (unless) */ linux_exit(-1); }
	test = lone_lisp_list_first(arguments);
	arguments = lone_lisp_list_rest(arguments);

	if (lone_lisp_is_nil(lone_lisp_evaluate(lone, module, environment, test))) {
		return lone_lisp_primitive_lone_begin(lone, module, environment, arguments, closure);
	}

	return lone_lisp_nil();
}

LONE_LISP_PRIMITIVE(lone_if)
{
	struct lone_lisp_value value, consequent, alternative;

	if (lone_lisp_is_nil(arguments)) { /* test not specified: (if) */ linux_exit(-1); }
	value = lone_lisp_list_first(arguments);
	arguments = lone_lisp_list_rest(arguments);

	if (lone_lisp_is_nil(arguments)) { /* consequent not specified: (if test) */ linux_exit(-1); }
	consequent = lone_lisp_list_first(arguments);
	arguments = lone_lisp_list_rest(arguments);

	alternative = lone_lisp_nil();

	if (!lone_lisp_is_nil(arguments)) {
		alternative = lone_lisp_list_first(arguments);
		arguments = lone_lisp_list_rest(arguments);
		if (!lone_lisp_is_nil(arguments)) { /* too many values (if test consequent alternative extra) */ linux_exit(-1); }
	}

	if (!lone_lisp_is_nil(lone_lisp_evaluate(lone, module, environment, value))) {
		return lone_lisp_evaluate(lone, module, environment, consequent);
	} else {
		return lone_lisp_evaluate(lone, module, environment, alternative);
	}
}

LONE_LISP_PRIMITIVE(lone_let)
{
	struct lone_lisp_value bindings, first, second, rest, value, new_environment;

	if (lone_lisp_is_nil(arguments)) { /* no variables to bind: (let) */ linux_exit(-1); }
	bindings = lone_lisp_list_first(arguments);
	if (!lone_lisp_is_list(bindings)) { /* expected list but got something else: (let 10) */ linux_exit(-1); }

	new_environment = lone_lisp_table_create(lone, 8, environment);

	while (1) {
		if (lone_lisp_is_nil(bindings)) { break; }
		first = lone_lisp_list_first(bindings);
		if (!lone_lisp_is_symbol(first)) { /* variable names must be symbols: (let ("x")) */ linux_exit(-1); }
		rest = lone_lisp_list_rest(bindings);
		if (lone_lisp_is_nil(rest)) { /* incomplete variable/value list: (let (x 10 y)) */ linux_exit(-1); }
		second = lone_lisp_list_first(rest);
		value = lone_lisp_evaluate(lone, module, new_environment, second);
		lone_lisp_table_set(lone, new_environment, first, value);
		bindings = lone_lisp_list_rest(rest);
	}

	value = lone_lisp_nil();

	while (!lone_lisp_is_nil(arguments = lone_lisp_list_rest(arguments))) {
		value = lone_lisp_evaluate(lone, module, new_environment, lone_lisp_list_first(arguments));
	}

	return value;
}

LONE_LISP_PRIMITIVE(lone_set)
{
	struct lone_lisp_value variable, value;

	if (lone_lisp_is_nil(arguments)) {
		/* no variable to set: (set) */
		linux_exit(-1);
	}

	variable = lone_lisp_list_first(arguments);
	if (!lone_lisp_is_symbol(variable)) {
		/* variable names must be symbols: (set 10) */
		linux_exit(-1);
	}

	arguments = lone_lisp_list_rest(arguments);
	if (lone_lisp_is_nil(arguments)) {
		/* value not specified: (set variable) */
		value = lone_lisp_nil();
	} else {
		/* (set variable value) */
		value = lone_lisp_list_first(arguments);
		arguments = lone_lisp_list_rest(arguments);
	}

	if (!lone_lisp_is_nil(arguments)) { /* too many arguments */ linux_exit(-1); }

	value = lone_lisp_evaluate(lone, module, environment, value);
	lone_lisp_table_set(lone, environment, variable, value);

	return value;
}

LONE_LISP_PRIMITIVE(lone_quote)
{
	struct lone_lisp_value argument;

	if (lone_lisp_list_destructure(arguments, 1, &argument)) {
		/* wrong number of arguments: (quote), (quote x y) */ linux_exit(-1);
	}

	return argument;
}

LONE_LISP_PRIMITIVE(lone_quasiquote)
{
	struct lone_lisp_value form, list, head, current, element, result, first, rest, unquote, splice;
	bool escaping, splicing;

	if (lone_lisp_list_destructure(arguments, 1, &form)) {
		/* wrong number of arguments: (quasiquote), (quasiquote x y) */ linux_exit(-1);
	}

	unquote = lone_lisp_intern_c_string(lone, "unquote");
	splice = lone_lisp_intern_c_string(lone, "unquote*");
	list = head = lone_lisp_nil();

	for (current = form; !lone_lisp_is_nil(current); current = lone_lisp_list_rest(current)) {
		element = lone_lisp_list_first(current);

		if (lone_lisp_is_list(element)) {
			first = lone_lisp_list_first(element);
			rest = lone_lisp_list_rest(element);

			if (lone_lisp_is_equivalent(first, unquote)) {
				escaping = true;
				splicing = false;
			} else if (lone_lisp_is_equivalent(first, splice)) {
				escaping = true;
				splicing = true;
			} else {
				escaping = false;
				splicing = false;
			}

			if (escaping) {
				first = lone_lisp_list_first(rest);
				rest = lone_lisp_list_rest(rest);

				if (!lone_lisp_is_nil(rest)) { /* too many arguments: (quasiquote (unquote x y) (unquote* x y)) */ linux_exit(-1); }

				result = lone_lisp_evaluate(lone, module, environment, first);

				if (splicing) {
					if (lone_lisp_is_list(result)) {
						for (/* result */; !lone_lisp_is_nil(result); result = lone_lisp_list_rest(result)) {
							lone_lisp_list_append(lone, &list, &head, lone_lisp_list_first(result));
						}
					} else {
						lone_lisp_list_append(lone, &list, &head, result);
					}

				} else {
					lone_lisp_list_append(lone, &list, &head, result);
				}

			} else {
				lone_lisp_list_append(lone, &list, &head, element);
			}

		} else {
			lone_lisp_list_append(lone, &list, &head, element);
		}
	}

	return list;
}

static struct lone_lisp_value lone_lisp_primitive_lambda_with_flags(struct lone_lisp *lone, struct lone_lisp_value environment, struct lone_lisp_value arguments, struct lone_lisp_function_flags flags)
{
	struct lone_lisp_value bindings, code;

	bindings = lone_lisp_list_first(arguments);
	if (!lone_lisp_is_list_or_nil(bindings)) { /* parameters not a list: (lambda 10) */ linux_exit(-1); }

	code = lone_lisp_list_rest(arguments);

	return lone_lisp_function_create(lone, bindings, code, environment, flags);
}

LONE_LISP_PRIMITIVE(lone_lambda)
{
	struct lone_lisp_function_flags flags = {
		.evaluate_arguments = 1,
		.evaluate_result = 0,
	};

	return lone_lisp_primitive_lambda_with_flags(lone, environment, arguments, flags);
}

LONE_LISP_PRIMITIVE(lone_lambda_bang)
{
	struct lone_lisp_function_flags flags = {
		.evaluate_arguments = 0,
		.evaluate_result = 0,
	};

	return lone_lisp_primitive_lambda_with_flags(lone, environment, arguments, flags);
}

LONE_LISP_PRIMITIVE(lone_is_list)
{
	return lone_lisp_apply_predicate(lone, arguments, lone_lisp_is_list);
}

LONE_LISP_PRIMITIVE(lone_is_vector)
{
	return lone_lisp_apply_predicate(lone, arguments, lone_lisp_is_vector);
}

LONE_LISP_PRIMITIVE(lone_is_table)
{
	return lone_lisp_apply_predicate(lone, arguments, lone_lisp_is_table);
}

LONE_LISP_PRIMITIVE(lone_is_symbol)
{
	return lone_lisp_apply_predicate(lone, arguments, lone_lisp_is_symbol);
}

LONE_LISP_PRIMITIVE(lone_is_text)
{
	return lone_lisp_apply_predicate(lone, arguments, lone_lisp_is_text);
}

LONE_LISP_PRIMITIVE(lone_is_integer)
{
	return lone_lisp_apply_predicate(lone, arguments, lone_lisp_is_integer);
}

LONE_LISP_PRIMITIVE(lone_is_identical)
{
	return lone_lisp_apply_comparator(lone, arguments, lone_lisp_is_identical);
}

LONE_LISP_PRIMITIVE(lone_is_equivalent)
{
	return lone_lisp_apply_comparator(lone, arguments, lone_lisp_is_equivalent);
}

LONE_LISP_PRIMITIVE(lone_is_equal)
{
	return lone_lisp_apply_comparator(lone, arguments, lone_lisp_is_equal);
}

LONE_LISP_PRIMITIVE(lone_print)
{
	while (!lone_lisp_is_nil(arguments)) {
		lone_lisp_print(lone, lone_lisp_list_first(arguments), 1);
		linux_write(1, "\n", 1);
		arguments = lone_lisp_list_rest(arguments);
	}

	return lone_lisp_nil();
}
