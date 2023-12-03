/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/modules.h>
#include <lone/modules/intrinsic/lone.h>

#include <lone/lisp/evaluator.h>
#include <lone/lisp/printer.h>
#include <lone/lisp/constants.h>
#include <lone/utilities.h>

#include <lone/value/function.h>
#include <lone/value/primitive.h>
#include <lone/value/list.h>
#include <lone/value/table.h>
#include <lone/value/symbol.h>

#include <lone/linux.h>

void lone_module_lone_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_intern_c_string(lone, "lone"),
	                  *module = lone_module_for_name(lone, name),
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = false, .evaluate_result = false };

	primitive = lone_primitive_create(lone, "begin", lone_primitive_lone_begin, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "begin"), primitive);

	primitive = lone_primitive_create(lone, "when", lone_primitive_lone_when, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "when"), primitive);

	primitive = lone_primitive_create(lone, "unless", lone_primitive_lone_unless, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "unless"), primitive);

	primitive = lone_primitive_create(lone, "if", lone_primitive_lone_if, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "if"), primitive);

	primitive = lone_primitive_create(lone, "let", lone_primitive_lone_let, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "let"), primitive);

	primitive = lone_primitive_create(lone, "set", lone_primitive_lone_set, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "set"), primitive);

	primitive = lone_primitive_create(lone, "quote", lone_primitive_lone_quote, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "quote"), primitive);

	primitive = lone_primitive_create(lone, "quasiquote", lone_primitive_lone_quasiquote, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "quasiquote"), primitive);

	primitive = lone_primitive_create(lone, "lambda", lone_primitive_lone_lambda, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "lambda"), primitive);

	primitive = lone_primitive_create(lone, "lambda_bang", lone_primitive_lone_lambda_bang, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "lambda!"), primitive);

	flags = (struct lone_function_flags) { .evaluate_arguments = true, .evaluate_result = false };

	primitive = lone_primitive_create(lone, "print", lone_primitive_lone_print, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "print"), primitive);

	primitive = lone_primitive_create(lone, "is_list", lone_primitive_lone_is_list, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "list?"), primitive);

	primitive = lone_primitive_create(lone, "is_vector", lone_primitive_lone_is_vector, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "vector?"), primitive);

	primitive = lone_primitive_create(lone, "is_table", lone_primitive_lone_is_table, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "table?"), primitive);

	primitive = lone_primitive_create(lone, "is_symbol", lone_primitive_lone_is_symbol, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "symbol?"), primitive);

	primitive = lone_primitive_create(lone, "is_text", lone_primitive_lone_is_text, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "text?"), primitive);

	primitive = lone_primitive_create(lone, "is_integer", lone_primitive_lone_is_integer, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "integer?"), primitive);

	primitive = lone_primitive_create(lone, "is_identical", lone_primitive_lone_is_identical, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "identical?"), primitive);

	primitive = lone_primitive_create(lone, "is_equivalent", lone_primitive_lone_is_equivalent, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "equivalent?"), primitive);

	primitive = lone_primitive_create(lone, "is_equal", lone_primitive_lone_is_equal, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "equal?"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}


LONE_PRIMITIVE(lone_begin)
{
	struct lone_value *value;

	for (value = lone_nil(lone); !lone_is_nil(arguments); arguments = lone_list_rest(arguments)) {
		value = lone_list_first(arguments);
		value = lone_evaluate(lone, module, environment, value);
	}

	return value;
}

LONE_PRIMITIVE(lone_when)
{
	struct lone_value *test;

	if (lone_is_nil(arguments)) { /* test not specified: (when) */ linux_exit(-1); }
	test = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);

	if (!lone_is_nil(lone_evaluate(lone, module, environment, test))) {
		return lone_primitive_lone_begin(lone, module, environment, arguments, closure);
	}

	return lone_nil(lone);
}

LONE_PRIMITIVE(lone_unless)
{
	struct lone_value *test;

	if (lone_is_nil(arguments)) { /* test not specified: (unless) */ linux_exit(-1); }
	test = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);

	if (lone_is_nil(lone_evaluate(lone, module, environment, test))) {
		return lone_primitive_lone_begin(lone, module, environment, arguments, closure);
	}

	return lone_nil(lone);
}

LONE_PRIMITIVE(lone_if)
{
	struct lone_value *value, *consequent, *alternative = 0;

	if (lone_is_nil(arguments)) { /* test not specified: (if) */ linux_exit(-1); }
	value = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);

	if (lone_is_nil(arguments)) { /* consequent not specified: (if test) */ linux_exit(-1); }
	consequent = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);

	if (!lone_is_nil(arguments)) {
		alternative = lone_list_first(arguments);
		arguments = lone_list_rest(arguments);
		if (!lone_is_nil(arguments)) { /* too many values (if test consequent alternative extra) */ linux_exit(-1); }
	}

	if (!lone_is_nil(lone_evaluate(lone, module, environment, value))) {
		return lone_evaluate(lone, module, environment, consequent);
	} else if (alternative) {
		return lone_evaluate(lone, module, environment, alternative);
	}

	return lone_nil(lone);
}

LONE_PRIMITIVE(lone_let)
{
	struct lone_value *bindings, *first, *second, *rest, *value, *new_environment;

	if (lone_is_nil(arguments)) { /* no variables to bind: (let) */ linux_exit(-1); }
	bindings = lone_list_first(arguments);
	if (!lone_is_list(bindings)) { /* expected list but got something else: (let 10) */ linux_exit(-1); }

	new_environment = lone_table_create(lone, 8, environment);

	while (1) {
		if (lone_is_nil(bindings)) { break; }
		first = lone_list_first(bindings);
		if (!lone_is_symbol(first)) { /* variable names must be symbols: (let ("x")) */ linux_exit(-1); }
		rest = lone_list_rest(bindings);
		if (lone_is_nil(rest)) { /* incomplete variable/value list: (let (x 10 y)) */ linux_exit(-1); }
		second = lone_list_first(rest);
		value = lone_evaluate(lone, module, new_environment, second);
		lone_table_set(lone, new_environment, first, value);
		bindings = lone_list_rest(rest);
	}

	value = lone_nil(lone);

	while (!lone_is_nil(arguments = lone_list_rest(arguments))) {
		value = lone_evaluate(lone, module, new_environment, lone_list_first(arguments));
	}

	return value;
}

LONE_PRIMITIVE(lone_set)
{
	struct lone_value *variable, *value;

	if (lone_is_nil(arguments)) {
		/* no variable to set: (set) */
		linux_exit(-1);
	}

	variable = lone_list_first(arguments);
	if (!lone_is_symbol(variable)) {
		/* variable names must be symbols: (set 10) */
		linux_exit(-1);
	}

	arguments = lone_list_rest(arguments);
	if (lone_is_nil(arguments)) {
		/* value not specified: (set variable) */
		value = lone_nil(lone);
	} else {
		/* (set variable value) */
		value = lone_list_first(arguments);
		arguments = lone_list_rest(arguments);
	}

	if (!lone_is_nil(arguments)) { /* too many arguments */ linux_exit(-1); }

	value = lone_evaluate(lone, module, environment, value);
	lone_table_set(lone, environment, variable, value);

	return value;
}

LONE_PRIMITIVE(lone_quote)
{
	if (!lone_is_nil(lone_list_rest(arguments))) { /* too many arguments: (quote x y) */ linux_exit(-1); }
	return lone_list_first(arguments);
}

LONE_PRIMITIVE(lone_quasiquote)
{
	struct lone_value *list, *head, *current, *element, *result, *first, *rest, *unquote, *splice;
	bool escaping, splicing;

	if (!lone_is_nil(lone_list_rest(arguments))) { /* too many arguments: (quasiquote x y) */ linux_exit(-1); }

	unquote = lone_intern_c_string(lone, "unquote");
	splice = lone_intern_c_string(lone, "unquote*");
	head = list = lone_list_create_nil(lone);
	arguments = lone_list_first(arguments);

	for (current = arguments; !lone_is_nil(current); current = lone_list_rest(current)) {
		element = lone_list_first(current);

		if (lone_is_list(element)) {
			first = lone_list_first(element);
			rest = lone_list_rest(element);

			if (lone_is_equivalent(first, unquote)) {
				escaping = true;
				splicing = false;
			} else if (lone_is_equivalent(first, splice)) {
				escaping = true;
				splicing = true;
			} else {
				escaping = false;
				splicing = false;
			}

			if (escaping) {
				first = lone_list_first(rest);
				rest = lone_list_rest(rest);

				if (!lone_is_nil(rest)) { /* too many arguments: (quasiquote (unquote x y) (unquote* x y)) */ linux_exit(-1); }

				result = lone_evaluate(lone, module, environment, first);

				if (splicing) {
					if (lone_is_list(result)) {
						for (/* result */; !lone_is_nil(result); result = lone_list_rest(result)) {
							head = lone_list_append(lone, head, lone_list_first(result));
						}
					} else {
						head = lone_list_append(lone, head, result);
					}

				} else {
					head = lone_list_append(lone, head, result);
				}

			} else {
				head = lone_list_append(lone, head, element);
			}

		} else {
			head = lone_list_append(lone, head, element);
		}
	}

	return list;
}

static struct lone_value *lone_primitive_lambda_with_flags(struct lone_lisp *lone, struct lone_value *environment, struct lone_value *arguments, struct lone_function_flags flags)
{
	struct lone_value *bindings, *code;

	bindings = lone_list_first(arguments);
	if (!lone_is_list(bindings)) { /* parameters not a list: (lambda 10) */ linux_exit(-1); }

	code = lone_list_rest(arguments);

	return lone_function_create(lone, bindings, code, environment, flags);
}

LONE_PRIMITIVE(lone_lambda)
{
	struct lone_function_flags flags = {
		.evaluate_arguments = 1,
		.evaluate_result = 0,
	};

	return lone_primitive_lambda_with_flags(lone, environment, arguments, flags);
}

LONE_PRIMITIVE(lone_lambda_bang)
{
	struct lone_function_flags flags = {
		.evaluate_arguments = 0,
		.evaluate_result = 0,
	};

	return lone_primitive_lambda_with_flags(lone, environment, arguments, flags);
}

LONE_PRIMITIVE(lone_is_list)
{
	return lone_apply_predicate(lone, arguments, lone_is_list);
}

LONE_PRIMITIVE(lone_is_vector)
{
	return lone_apply_predicate(lone, arguments, lone_is_vector);
}

LONE_PRIMITIVE(lone_is_table)
{
	return lone_apply_predicate(lone, arguments, lone_is_table);
}

LONE_PRIMITIVE(lone_is_symbol)
{
	return lone_apply_predicate(lone, arguments, lone_is_symbol);
}

LONE_PRIMITIVE(lone_is_text)
{
	return lone_apply_predicate(lone, arguments, lone_is_text);
}

LONE_PRIMITIVE(lone_is_integer)
{
	return lone_apply_predicate(lone, arguments, lone_is_integer);
}

LONE_PRIMITIVE(lone_is_identical)
{
	return lone_apply_comparator(lone, arguments, lone_is_identical);
}

LONE_PRIMITIVE(lone_is_equivalent)
{
	return lone_apply_comparator(lone, arguments, lone_is_equivalent);
}

LONE_PRIMITIVE(lone_is_equal)
{
	return lone_apply_comparator(lone, arguments, lone_is_equal);
}

LONE_PRIMITIVE(lone_print)
{
	while (!lone_is_nil(arguments)) {
		lone_print(lone, lone_list_first(arguments), 1);
		linux_write(1, "\n", 1);
		arguments = lone_list_rest(arguments);
	}

	return lone_nil(lone);
}
