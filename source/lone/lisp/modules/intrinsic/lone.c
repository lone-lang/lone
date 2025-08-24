/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/lone.h>

#include <lone/lisp/machine.h>
#include <lone/lisp/module.h>
#include <lone/lisp/printer.h>
#include <lone/lisp/utilities.h>

#include <lone/lisp/value/function.h>
#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/symbol.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_lone_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_value name, module;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "lone");
	module = lone_lisp_module_for_name(lone, name);
	flags.evaluate_arguments = false;
	flags.evaluate_result = false;

	lone_lisp_module_export_primitive(lone, module, "begin",
			"begin", lone_lisp_primitive_lone_begin, module, flags);

	lone_lisp_module_export_primitive(lone, module, "when",
			"when", lone_lisp_primitive_lone_when, module, flags);

	lone_lisp_module_export_primitive(lone, module, "unless",
			"unless", lone_lisp_primitive_lone_unless, module, flags);

	lone_lisp_module_export_primitive(lone, module, "if",
			"if", lone_lisp_primitive_lone_if, module, flags);

	lone_lisp_module_export_primitive(lone, module, "let",
			"let", lone_lisp_primitive_lone_let, module, flags);

	lone_lisp_module_export_primitive(lone, module, "set",
			"set", lone_lisp_primitive_lone_set, module, flags);

	lone_lisp_module_export_primitive(lone, module, "quote",
			"quote", lone_lisp_primitive_lone_quote, module, flags);

	lone_lisp_module_export_primitive(lone, module, "quasiquote",
			"quasiquote", lone_lisp_primitive_lone_quasiquote, module, flags);

	lone_lisp_module_export_primitive(lone, module, "lambda",
			"lambda", lone_lisp_primitive_lone_lambda, module, flags);

	lone_lisp_module_export_primitive(lone, module, "lambda!",
			"lambda_bang", lone_lisp_primitive_lone_lambda_bang, module, flags);

	flags = (struct lone_lisp_function_flags) { .evaluate_arguments = true, .evaluate_result = false };

	lone_lisp_module_export_primitive(lone, module, "print",
			"print", lone_lisp_primitive_lone_print, module, flags);

	lone_lisp_module_export_primitive(lone, module, "list?",
			"is_list", lone_lisp_primitive_lone_is_list, module, flags);

	lone_lisp_module_export_primitive(lone, module, "vector?",
			"is_vector", lone_lisp_primitive_lone_is_vector, module, flags);

	lone_lisp_module_export_primitive(lone, module, "table?",
			"is_table", lone_lisp_primitive_lone_is_table, module, flags);

	lone_lisp_module_export_primitive(lone, module, "symbol?",
			"is_symbol", lone_lisp_primitive_lone_is_symbol, module, flags);

	lone_lisp_module_export_primitive(lone, module, "text?",
			"is_text", lone_lisp_primitive_lone_is_text, module, flags);

	lone_lisp_module_export_primitive(lone, module, "integer?",
			"is_integer", lone_lisp_primitive_lone_is_integer, module, flags);

	lone_lisp_module_export_primitive(lone, module, "identical?",
			"is_identical", lone_lisp_primitive_lone_is_identical, module, flags);

	lone_lisp_module_export_primitive(lone, module, "equivalent?",
			"is_equivalent", lone_lisp_primitive_lone_is_equivalent, module, flags);

	lone_lisp_module_export_primitive(lone, module, "equal?",
			"is_equal", lone_lisp_primitive_lone_is_equal, module, flags);
}


LONE_LISP_PRIMITIVE(lone_begin)
{
	struct lone_lisp_value expressions;

	switch (step) {
	case 0: /* unpack arguments and setup */

		expressions = lone_lisp_machine_pop_value(lone, machine);

		machine->step = LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_FROM_PRIMITIVE;
		machine->unevaluated = expressions;
		return 1;

	case 1: /* collect and return result */

		lone_lisp_machine_push_value(lone, machine, machine->value);
		return 0;

	default:
		break;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(lone_when)
{
	struct lone_lisp_value arguments, condition, body;

	switch (step) {
	case 0: /* unpack and check arguments then evaluate condition */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_is_nil(arguments)) { /* condition not specified: (when) */ linux_exit(-1); }
		condition = lone_lisp_list_first(arguments);
		body = lone_lisp_list_rest(arguments);

		lone_lisp_machine_push_value(lone, machine, body);

		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		machine->expression = condition;

		return 1;

	case 1: /* evaluate remaining expressions if condition true */

		body = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_is_truthy(machine->value)) {
			machine->step = LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_FROM_PRIMITIVE;
			machine->unevaluated = body;
			return 2;
		} else {
			lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
			return 0;
		}

	case 2: /* collect result of sequence evaluation and return it */
		lone_lisp_machine_push_value(lone, machine, machine->value);
		return 0;

	default:
		break;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(lone_unless)
{
	struct lone_lisp_value arguments, condition, body;

	switch (step) {
	case 0: /* unpack and check arguments then evaluate condition */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_is_nil(arguments)) { /* condition not specified: (unless) */ linux_exit(-1); }
		condition = lone_lisp_list_first(arguments);
		body = lone_lisp_list_rest(arguments);

		lone_lisp_machine_push_value(lone, machine, body);

		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		machine->expression = condition;
		return 1;

	case 1: /* evaluate remaining expressions if condition false */

		body = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_is_falsy(machine->value)) {
			machine->step = LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_FROM_PRIMITIVE;
			machine->unevaluated = body;
			return 2;
		} else {
			lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
			return 0;
		}

	case 2: /* collect result of sequence evaluation and return it */
		lone_lisp_machine_push_value(lone, machine, machine->value);
		return 0;

	default:
		break;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(lone_if)
{
	struct lone_lisp_value arguments, condition, consequent, alternative;

	switch (step) {
	case 0: /* unpack and check arguments, then evaluate condition */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_is_nil(arguments)) { /* test not specified: (if) */ linux_exit(-1); }
		condition = lone_lisp_list_first(arguments);
		arguments = lone_lisp_list_rest(arguments);

		if (lone_lisp_is_nil(arguments)) { /* consequent not specified: (if test) */ linux_exit(-1); }
		consequent = lone_lisp_list_first(arguments);
		arguments = lone_lisp_list_rest(arguments);

		alternative = lone_lisp_nil();

		if (!lone_lisp_is_nil(arguments)) {
			alternative = lone_lisp_list_first(arguments);
			arguments = lone_lisp_list_rest(arguments);
			if (!lone_lisp_is_nil(arguments)) {
				/* too many values (if test consequent alternative extra) */ linux_exit(-1);
			}
		}

		lone_lisp_machine_push_value(lone, machine, alternative);
		lone_lisp_machine_push_value(lone, machine, consequent);

		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		machine->expression = condition;
		return 1;

	case 1: /* check evaluated condition and then evaluate consequent or alternative */

		consequent  = lone_lisp_machine_pop_value(lone, machine);
		alternative = lone_lisp_machine_pop_value(lone, machine);

		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		machine->expression = lone_lisp_is_truthy(machine->value)? consequent : alternative;
		return 2;

	case 2: /* collect result and return the value */
		lone_lisp_machine_push_value(lone, machine, machine->value);
		return 0;

	default:
		break;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(lone_let)
{
	struct lone_lisp_value arguments, bindings, body,
	                       original_environment, new_environment,
	                       first, second, rest, value;

	switch (step) {
	case 0: /* unpack and check arguments, evaluate values and bind them to variables */

		arguments = lone_lisp_machine_pop_value(lone, machine);
		if (lone_lisp_is_nil(arguments)) { /* no variables to bind: (let) */ linux_exit(-1); }

		bindings = lone_lisp_list_first(arguments);
		if (!lone_lisp_is_list(bindings)) { /* expected list but got something else: (let 10) */ linux_exit(-1); }

		body = lone_lisp_list_rest(arguments);
		original_environment = machine->environment;
		new_environment = lone_lisp_table_create(lone, 8, original_environment);

		while (1) {
			if (lone_lisp_is_nil(bindings)) { break; }

			first = lone_lisp_list_first(bindings);
			if (!lone_lisp_is_symbol(first)) {
				/* variable names must be symbols: (let ("x")) */ linux_exit(-1);
			}

			rest = lone_lisp_list_rest(bindings);
			if (lone_lisp_is_nil(rest)) {
				/* incomplete variable/value list: (let (x 10 y)) */ linux_exit(-1);
			}

			second = lone_lisp_list_first(rest);

			lone_lisp_machine_push_value(lone, machine, rest);
			lone_lisp_machine_push_value(lone, machine, first);
			lone_lisp_machine_push_value(lone, machine, original_environment);
			lone_lisp_machine_push_value(lone, machine, new_environment);
			lone_lisp_machine_push_value(lone, machine, body);

			machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
			machine->expression = second;
			machine->environment = new_environment;
			return 1;

	bind_value_to_variable:
			lone_lisp_table_set(lone, new_environment, first, value);
			bindings = lone_lisp_list_rest(rest);
		}

		lone_lisp_machine_push_value(lone, machine, original_environment);
		machine->environment = new_environment;
		machine->step = LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_FROM_PRIMITIVE;
		machine->unevaluated = body;
		return 2;

	case 1: /* collect evaluated value for variable binding */

		body                 = lone_lisp_machine_pop_value(lone, machine);
		new_environment      = lone_lisp_machine_pop_value(lone, machine);
		original_environment = lone_lisp_machine_pop_value(lone, machine);
		first                = lone_lisp_machine_pop_value(lone, machine);
		rest                 = lone_lisp_machine_pop_value(lone, machine);
		value                = machine->value;

		goto bind_value_to_variable;

	case 2: /* restore environment; collect and return result of last evaluated expression */

		machine->environment = lone_lisp_machine_pop_value(lone, machine);
		lone_lisp_machine_push_value(lone, machine, machine->value);
		return 0;

	default:
		break;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(lone_set)
{
	struct lone_lisp_value arguments, variable, value;

	switch (step) {
	case 0: /* unpack and check arguments */

		arguments = lone_lisp_machine_pop_value(lone, machine);
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
			goto set_value;
		} else {
			/* (set variable value) */
			value = lone_lisp_list_first(arguments);
			arguments = lone_lisp_list_rest(arguments);
		}

		if (!lone_lisp_is_nil(arguments)) { /* too many arguments */ linux_exit(-1); }

		lone_lisp_machine_push_value(lone, machine, variable);

		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		machine->expression = value;
		return 1;

	case 1: /* value has been evaluated */

		variable = lone_lisp_machine_pop_value(lone, machine);
		value    = machine->value;

	set_value:
		lone_lisp_table_set(lone, machine->environment, variable, value);
		lone_lisp_machine_push_value(lone, machine, value);
		return 0;

	default:
		break;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(lone_quote)
{
	struct lone_lisp_value arguments, argument;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(arguments, 1, &argument)) {
		/* wrong number of arguments: (quote), (quote x y) */ linux_exit(-1);
	}

	lone_lisp_machine_push_value(lone, machine, argument);
	return 0;
}

LONE_LISP_PRIMITIVE(lone_quasiquote)
{
	struct lone_lisp_value arguments, form, list, head, current, element, result, first, rest;
	struct lone_lisp_value unquote, splice, escaping, splicing;

	switch (step) {
	case 0: /* unpack and check arguments then loop */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(arguments, 1, &form)) {
			/* wrong number of arguments: (quasiquote), (quasiquote x y) */ linux_exit(-1);
		}

		unquote = lone_lisp_intern_c_string(lone, "unquote");
		splice = lone_lisp_intern_c_string(lone, "unquote*");
		list = head = lone_lisp_nil();
		current = form;

		for (current = form; !lone_lisp_is_nil(current); current = lone_lisp_list_rest(current)) {
			element = lone_lisp_list_first(current);

			if (lone_lisp_is_list(element)) {
				first = lone_lisp_list_first(element);
				rest = lone_lisp_list_rest(element);

				if (lone_lisp_is_equivalent(first, unquote)) {
					escaping = lone_lisp_true();
					splicing = lone_lisp_false();
				} else if (lone_lisp_is_equivalent(first, splice)) {
					escaping = lone_lisp_true();
					splicing = lone_lisp_true();
				} else {
					escaping = lone_lisp_false();
					splicing = lone_lisp_false();
				}

				if (lone_lisp_is_true(escaping)) {
					first = lone_lisp_list_first(rest);
					rest = lone_lisp_list_rest(rest);

					if (!lone_lisp_is_nil(rest)) {
						/* too many arguments: (quasiquote (unquote x y) (unquote* x y)) */
						linux_exit(-1);
					}

					lone_lisp_machine_push_value(lone, machine, unquote);
					lone_lisp_machine_push_value(lone, machine, splice);
					lone_lisp_machine_push_value(lone, machine, splicing);
					lone_lisp_machine_push_value(lone, machine, current);
					lone_lisp_machine_push_value(lone, machine, head);
					lone_lisp_machine_push_value(lone, machine, list);

					machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
					machine->expression = first;
					return 1;

	after_evaluation:
					if (lone_lisp_is_true(splicing)) {
						if (lone_lisp_is_list(result)) {
							for (/* result */;
									!lone_lisp_is_nil(result);
									result = lone_lisp_list_rest(result)) {
								lone_lisp_list_append(lone, &list, &head,
										lone_lisp_list_first(result));
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

	early_exit:
		lone_lisp_machine_push_value(lone, machine, list);
		return 0;

	case 1: /* collect result and resume loop */

		list     = lone_lisp_machine_pop_value(lone, machine);
		head     = lone_lisp_machine_pop_value(lone, machine);
		current  = lone_lisp_machine_pop_value(lone, machine);
		splicing = lone_lisp_machine_pop_value(lone, machine);
		splice   = lone_lisp_machine_pop_value(lone, machine);
		unquote  = lone_lisp_machine_pop_value(lone, machine);

		result = machine->value;

		goto after_evaluation;

	default:
		break;
	}

	linux_exit(-1);
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

	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_primitive_lambda_with_flags(lone,
				machine->environment, lone_lisp_machine_pop_value(lone, machine), flags));
	return 0;
}

LONE_LISP_PRIMITIVE(lone_lambda_bang)
{
	struct lone_lisp_function_flags flags = {
		.evaluate_arguments = 0,
		.evaluate_result = 0,
	};

	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_primitive_lambda_with_flags(lone,
				machine->environment, lone_lisp_machine_pop_value(lone, machine), flags));
	return 0;
}

LONE_LISP_PRIMITIVE(lone_is_list)
{
	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_apply_predicate(lone,
				lone_lisp_machine_pop_value(lone, machine), lone_lisp_is_list));
	return 0;
}

LONE_LISP_PRIMITIVE(lone_is_vector)
{
	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_apply_predicate(lone,
				lone_lisp_machine_pop_value(lone, machine), lone_lisp_is_vector));
	return 0;
}

LONE_LISP_PRIMITIVE(lone_is_table)
{
	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_apply_predicate(lone,
				lone_lisp_machine_pop_value(lone, machine), lone_lisp_is_table));
	return 0;
}

LONE_LISP_PRIMITIVE(lone_is_symbol)
{
	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_apply_predicate(lone,
				lone_lisp_machine_pop_value(lone, machine), lone_lisp_is_symbol));
	return 0;
}

LONE_LISP_PRIMITIVE(lone_is_text)
{
	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_apply_predicate(lone,
				lone_lisp_machine_pop_value(lone, machine), lone_lisp_is_text));
	return 0;
}

LONE_LISP_PRIMITIVE(lone_is_integer)
{
	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_apply_predicate(lone,
				lone_lisp_machine_pop_value(lone, machine), lone_lisp_is_integer));
	return 0;
}

LONE_LISP_PRIMITIVE(lone_is_identical)
{
	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_apply_comparator(lone,
				lone_lisp_machine_pop_value(lone, machine), lone_lisp_is_identical));
	return 0;
}

LONE_LISP_PRIMITIVE(lone_is_equivalent)
{
	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_apply_comparator(lone,
				lone_lisp_machine_pop_value(lone, machine), lone_lisp_is_equivalent));
	return 0;
}

LONE_LISP_PRIMITIVE(lone_is_equal)
{
	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_apply_comparator(lone,
				lone_lisp_machine_pop_value(lone, machine), lone_lisp_is_equal));
	return 0;
}

LONE_LISP_PRIMITIVE(lone_print)
{
	struct lone_lisp_value arguments;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	while (!lone_lisp_is_nil(arguments)) {
		lone_lisp_print(lone, lone_lisp_list_first(arguments), 1);
		linux_write(1, "\n", 1);
		arguments = lone_lisp_list_rest(arguments);
	}

	lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
	return 0;
}
