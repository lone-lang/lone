/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/lone.h>

#include <lone/lisp/machine.h>
#include <lone/lisp/machine/stack.h>
#include <lone/lisp/module.h>
#include <lone/lisp/printer.h>
#include <lone/lisp/utilities.h>

#include <lone/memory/array.h>
#include <lone/memory/functions.h>

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

	lone_lisp_module_export_primitive(lone, module, "control",
			"control", lone_lisp_primitive_lone_control, module, flags);

	lone_lisp_module_export_primitive(lone, module, "intercept",
			"intercept", lone_lisp_primitive_lone_intercept, module, flags);

	flags = (struct lone_lisp_function_flags) { .evaluate_arguments = true, .evaluate_result = false };

	lone_lisp_module_export_primitive(lone, module, "return",
			"return", lone_lisp_primitive_lone_return, module, flags);

	lone_lisp_module_export_primitive(lone, module, "transfer",
			"transfer", lone_lisp_primitive_lone_transfer, module, flags);

	lone_lisp_module_export_primitive(lone, module, "generator",
			"generator", lone_lisp_primitive_lone_generator, module, flags);

	lone_lisp_module_export_primitive(lone, module, "yield",
			"yield", lone_lisp_primitive_lone_yield, module, flags);

	lone_lisp_module_export_primitive(lone, module, "signal",
			"signal", lone_lisp_primitive_lone_signal, module, flags);

	lone_lisp_module_export_primitive(lone, module, "print",
			"print", lone_lisp_primitive_lone_print, module, flags);

	lone_lisp_module_export_primitive(lone, module, "nil?",
			"is_nil", lone_lisp_primitive_lone_is_nil, module, flags);

	lone_lisp_module_export_primitive(lone, module, "true?",
			"is_true", lone_lisp_primitive_lone_is_true, module, flags);

	lone_lisp_module_export_primitive(lone, module, "false?",
			"is_false", lone_lisp_primitive_lone_is_false, module, flags);

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
	case 0:

		expressions = lone_lisp_machine_pop_value(lone, machine);
		goto evaluate_next;

	case 1:

		expressions = lone_lisp_machine_pop_value(lone, machine);

	evaluate_next:

		if (!lone_lisp_list_has_rest(lone, expressions)) {
			/* last or only expression: tail return */
			machine->expression = lone_lisp_list_first(lone, expressions);
			return -1;
		}

		/* more expressions follow: evaluate this one, continue with rest */
		lone_lisp_machine_push_value(lone, machine, lone_lisp_list_rest(lone, expressions));
		machine->expression = lone_lisp_list_first(lone, expressions);
		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		return 1;

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
		condition = lone_lisp_list_first(lone, arguments);
		body = lone_lisp_list_rest(lone, arguments);

		lone_lisp_machine_push_value(lone, machine, body);

		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		machine->expression = condition;

		return 1;

	case 1: /* evaluate body if condition true */

		body = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_is_falsy(machine->value)) {
			lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
			return 0;
		}

		goto evaluate_body;

	case 2:

		body = lone_lisp_machine_pop_value(lone, machine);

	evaluate_body:

		if (!lone_lisp_list_has_rest(lone, body)) {
			/* last or only expression: tail return */
			machine->expression = lone_lisp_list_first(lone, body);
			return -1;
		}

		/* more expressions follow: evaluate this one, continue with rest */
		lone_lisp_machine_push_value(lone, machine, lone_lisp_list_rest(lone, body));
		machine->expression = lone_lisp_list_first(lone, body);
		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		return 2;

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
		condition = lone_lisp_list_first(lone, arguments);
		body = lone_lisp_list_rest(lone, arguments);

		lone_lisp_machine_push_value(lone, machine, body);

		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		machine->expression = condition;
		return 1;

	case 1: /* evaluate body if condition false */

		body = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_is_truthy(machine->value)) {
			lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
			return 0;
		}

		goto evaluate_body;

	case 2:

		body = lone_lisp_machine_pop_value(lone, machine);

	evaluate_body:

		if (!lone_lisp_list_has_rest(lone, body)) {
			/* last or only expression: tail return */
			machine->expression = lone_lisp_list_first(lone, body);
			return -1;
		}

		/* more expressions follow: evaluate this one, continue with rest */
		lone_lisp_machine_push_value(lone, machine, lone_lisp_list_rest(lone, body));
		machine->expression = lone_lisp_list_first(lone, body);
		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		return 2;

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
		condition = lone_lisp_list_first(lone, arguments);
		arguments = lone_lisp_list_rest(lone, arguments);

		if (lone_lisp_is_nil(arguments)) { /* consequent not specified: (if test) */ linux_exit(-1); }
		consequent = lone_lisp_list_first(lone, arguments);
		arguments = lone_lisp_list_rest(lone, arguments);

		alternative = lone_lisp_nil();

		if (!lone_lisp_is_nil(arguments)) {
			alternative = lone_lisp_list_first(lone, arguments);
			arguments = lone_lisp_list_rest(lone, arguments);
			if (!lone_lisp_is_nil(arguments)) {
				/* too many values (if test consequent alternative extra) */ linux_exit(-1);
			}
		}

		lone_lisp_machine_push_value(lone, machine, alternative);
		lone_lisp_machine_push_value(lone, machine, consequent);

		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		machine->expression = condition;
		return 1;

	case 1: /* check evaluated condition and tail return consequent or alternative */

		consequent  = lone_lisp_machine_pop_value(lone, machine);
		alternative = lone_lisp_machine_pop_value(lone, machine);

		machine->expression = lone_lisp_is_truthy(machine->value)? consequent : alternative;
		return -1;

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

		bindings = lone_lisp_list_first(lone, arguments);
		if (!lone_lisp_is_list(lone, bindings)) {
			/* expected list but got something else: (let 10) */ linux_exit(-1);
		}

		body = lone_lisp_list_rest(lone, arguments);
		original_environment = machine->environment;
		new_environment = lone_lisp_table_create(lone, 8, original_environment);

		while (1) {
			if (lone_lisp_is_nil(bindings)) { break; }

			first = lone_lisp_list_first(lone, bindings);
			if (!lone_lisp_is_symbol(lone, first)) {
				/* variable names must be symbols: (let ("x")) */ linux_exit(-1);
			}

			rest = lone_lisp_list_rest(lone, bindings);
			if (lone_lisp_is_nil(rest)) {
				/* incomplete variable/value list: (let (x 10 y)) */ linux_exit(-1);
			}

			second = lone_lisp_list_first(lone, rest);

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
			bindings = lone_lisp_list_rest(lone, rest);
		}

		machine->environment = new_environment;
		goto evaluate_body;

	case 1: /* collect evaluated value for variable binding */

		body                 = lone_lisp_machine_pop_value(lone, machine);
		new_environment      = lone_lisp_machine_pop_value(lone, machine);
		original_environment = lone_lisp_machine_pop_value(lone, machine);
		first                = lone_lisp_machine_pop_value(lone, machine);
		rest                 = lone_lisp_machine_pop_value(lone, machine);
		value                = machine->value;

		goto bind_value_to_variable;

	case 2:

		body = lone_lisp_machine_pop_value(lone, machine);

	evaluate_body:

		if (lone_lisp_is_nil(body)) {
			/* empty body: (let (x 10)) → nil */
			lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
			return 0;
		}

		if (!lone_lisp_list_has_rest(lone, body)) {
			/* last or only expression: tail return */
			machine->expression = lone_lisp_list_first(lone, body);
			return -1;
		}

		/* more expressions follow: evaluate this one, continue with rest */
		lone_lisp_machine_push_value(lone, machine, lone_lisp_list_rest(lone, body));
		machine->expression = lone_lisp_list_first(lone, body);
		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		return 2;

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

		variable = lone_lisp_list_first(lone, arguments);
		if (!lone_lisp_is_symbol(lone, variable)) {
			/* variable names must be symbols: (set 10) */
			linux_exit(-1);
		}

		arguments = lone_lisp_list_rest(lone, arguments);
		if (lone_lisp_is_nil(arguments)) {
			/* value not specified: (set variable) */
			value = lone_lisp_nil();
			goto set_value;
		} else {
			/* (set variable value) */
			value = lone_lisp_list_first(lone, arguments);
			arguments = lone_lisp_list_rest(lone, arguments);
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

	if (lone_lisp_list_destructure(lone, arguments, 1, &argument)) {
		/* wrong number of arguments: (quote), (quote x y) */ linux_exit(-1);
	}

	lone_lisp_machine_push_value(lone, machine, argument);
	return 0;
}

LONE_LISP_PRIMITIVE(lone_quasiquote)
{
	struct lone_lisp_value arguments, form, list, current, element, result, first, rest;
	struct lone_lisp_value unquote, splice, escaping, splicing;
	lone_lisp_integer count, i;

	switch (step) {
	case 0: /* unpack and check arguments then loop */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(lone, arguments, 1, &form)) {
			/* wrong number of arguments: (quasiquote), (quasiquote x y) */ linux_exit(-1);
		}

		unquote = lone_lisp_intern_c_string(lone, "unquote");
		splice = lone_lisp_intern_c_string(lone, "unquote*");
		current = form;
		count = 0;

		for (current = form; !lone_lisp_is_nil(current); current = lone_lisp_list_rest(lone, current)) {
			element = lone_lisp_list_first(lone, current);

			if (lone_lisp_is_list(lone, element)) {
				first = lone_lisp_list_first(lone, element);
				rest = lone_lisp_list_rest(lone, element);

				if (lone_lisp_is_equivalent(lone, first, unquote)) {
					escaping = lone_lisp_true();
					splicing = lone_lisp_false();
				} else if (lone_lisp_is_equivalent(lone, first, splice)) {
					escaping = lone_lisp_true();
					splicing = lone_lisp_true();
				} else {
					escaping = lone_lisp_false();
					splicing = lone_lisp_false();
				}

				if (lone_lisp_is_true(escaping)) {
					first = lone_lisp_list_first(lone, rest);
					rest = lone_lisp_list_rest(lone, rest);

					if (!lone_lisp_is_nil(rest)) {
						/* too many arguments: (quasiquote (unquote x y) (unquote* x y)) */
						linux_exit(-1);
					}

					lone_lisp_machine_push_integer(lone, machine, count);
					lone_lisp_machine_push_value(lone, machine, unquote);
					lone_lisp_machine_push_value(lone, machine, splice);
					lone_lisp_machine_push_value(lone, machine, splicing);
					lone_lisp_machine_push_value(lone, machine, current);

					machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
					machine->expression = first;
					return 1;

	after_evaluation:
					if (lone_lisp_is_true(splicing)) {
						if (lone_lisp_is_list(lone, result)) {
							for (/* result */;
									!lone_lisp_is_nil(result);
									result = lone_lisp_list_rest(lone, result)) {
								lone_lisp_machine_push_value(
									lone,
									machine,
									lone_lisp_list_first(lone, result)
								);
								++count;
							}
						} else {
							lone_lisp_machine_push_value(
								lone,
								machine,
								result
							);
							++count;
						}
					} else {
						lone_lisp_machine_push_value(
							lone,
							machine,
							result
						);
						++count;
					}
				} else {
					lone_lisp_machine_push_value(
						lone,
						machine,
						element
					);
					++count;
				}
			} else {
				lone_lisp_machine_push_value(
					lone,
					machine,
					element
				);
				++count;
			}
		}

		list = lone_lisp_nil();
		for (i = 0; i < count; ++i) {
			list =
				lone_lisp_list_create(
					lone,
					lone_lisp_machine_pop_value(lone, machine),
					list
				);
		}

		lone_lisp_machine_push_value(lone, machine, list);
		return 0;

	case 1: /* collect result and resume loop */

		current  = lone_lisp_machine_pop_value(lone, machine);
		splicing = lone_lisp_machine_pop_value(lone, machine);
		splice   = lone_lisp_machine_pop_value(lone, machine);
		unquote  = lone_lisp_machine_pop_value(lone, machine);
		count    = lone_lisp_machine_pop_integer(lone, machine);

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

	bindings = lone_lisp_list_first(lone, arguments);
	if (!lone_lisp_is_list(lone, bindings)) { /* parameters not a list: (lambda 10) */ linux_exit(-1); }

	code = lone_lisp_list_rest(lone, arguments);

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

LONE_LISP_PRIMITIVE(lone_return)
{
	struct lone_lisp_value return_value;

	return_value = lone_lisp_list_first(lone, lone_lisp_machine_pop_value(lone, machine));

	lone_lisp_machine_pop_primitive_delimiter(lone, machine); // this primitive's own delimiter
	lone_lisp_machine_unwind_to_primitive_delimiter(lone, machine);

	lone_lisp_machine_push_value(lone, machine, return_value);
	return 0;
}

LONE_LISP_PRIMITIVE(lone_control)
{
	struct lone_lisp_value arguments, body, handler;

	switch (step) {
	case 0: /* unpack arguments then evaluate body */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(lone, arguments, 2, &body, &handler)) {
			/* wrong number of arguments: (control), (control body handler extra) */ linux_exit(-1);
		}

		lone_lisp_machine_push_value(lone, machine, handler);
		lone_lisp_machine_push_continuation_delimiter(lone, machine);

		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		machine->expression = body;
		return 1;

	case 1: /* body evaluated */

		lone_lisp_machine_pop_continuation_delimiter(lone, machine);
		lone_lisp_machine_pop_value(lone, machine); /* handler */
		lone_lisp_machine_push_value(lone, machine, machine->value); /* return value */
		return 0;

	default:
		break;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(lone_transfer)
{
	struct lone_lisp_machine_stack_frame *frame, *frames;
	struct lone_lisp_value arguments, value, continuation, handler;
	size_t frame_count;

	switch (step) {
	case 0: /* unpack arguments, capture continuation, reset stack and evaluate handler */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(lone, arguments, 1, &value)) {
			/* wrong number of arguments: (transfer), (transfer value extra) */ linux_exit(-1);
		}

		/* skip primitive function delimiter and one step,
		   search for the continuation delimiter pushed by control */
		for (frame = machine->stack.top - 1 - 2,
		     frame_count = 1;
		     frame >= machine->stack.base;
		     --frame, ++frame_count) {

			switch (frame->tagged & LONE_LISP_TAG_MASK) {
			case LONE_LISP_TAG_CONTINUATION_DELIMITER:
				goto found;
			case LONE_LISP_TAG_GENERATOR_DELIMITER:
				/* continuation capture across generator boundaries
				   is not supported: disjoint stacks cannot be spliced */
				linux_exit(-1);
			default:
				continue;
			}
		}

		/* reached stack base without finding a continuation delimiter:
		   transfer called without a matching control */
		linux_exit(-1);

	found:

		/* recover the handler function */
		--frame; ++frame_count;
		handler = (struct lone_lisp_value) { .tagged = frame->tagged };

		/* copy stack frames up to and including the control primitive's function delimiter */
		--frame; ++frame_count;
		frames = lone_memory_array(lone->system, 0, 0, frame_count, sizeof(*frames), alignof(*frames));
		lone_memory_move(frame, frames, lone_memory_array_size_in_bytes(frame_count, sizeof(*frames)));

		/* reify current continuation */
		continuation = lone_lisp_continuation_create(lone, frame_count, frames);

		/* reset stack back to the control primitive's function delimiter */
		machine->stack.top = frame + 1;

		/* configure machine to evaluate handler function with value and continuation */
		machine->expression = lone_lisp_list_build(lone, 3, &handler, &value, &continuation);
		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;

		return 1;

	case 1:

		/* handler has finished evaluation, return the value returned by it */
		lone_lisp_machine_push_value(lone, machine, machine->value);
		return 0;

	default:
		break;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(lone_generator)
{
	struct lone_lisp_value arguments, function, generator;

	switch (step) {
	case 0: /* unpack arguments then create generator */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(lone, arguments, 1, &function)) {
			/* wrong number of arguments: (generator), (generator function extra) */ linux_exit(-1);
		}

		if (!lone_lisp_is_applicable(lone, function)) {
			/* not passed a function: (generator 10) */ linux_exit(-1);
		}

		generator = lone_lisp_generator_create(lone, function, LONE_LISP_GENERATOR_STACK_INITIAL_SIZE);

		lone_lisp_machine_push_value(lone, machine, generator);
		return 0;

	default:
		break;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(lone_yield)
{
	struct lone_lisp_machine_stack_frame *delimiter;
	struct lone_lisp_value arguments, value;
	struct lone_lisp_generator *generator;

	switch (step) {
	case 0: /* unpack arguments, find generator, swap stacks, return yielded value */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(lone, arguments, 1, &value)) {
			value = arguments;
		}

		/* generator delimiter is in a fixed position on the generator's stack */
		delimiter = &machine->stack.base[0];
		if ((delimiter->tagged & LONE_LISP_TAG_MASK) != LONE_LISP_TAG_GENERATOR_DELIMITER) {
			/* not inside a generator */ linux_exit(-1);
		}
		generator = &lone_lisp_heap_value_of(
			lone,
			lone_lisp_retag_frame(*delimiter, LONE_LISP_TAG_GENERATOR)
		)->as.generator;

		/* save the generator's stack */
		generator->stacks.own = machine->stack;

		/* restore the caller's stack */
		machine->stack = generator->stacks.caller;

		/* mark generator as suspended by clearing the caller stack */
		generator->stacks.caller = (struct lone_lisp_machine_stack) { 0 };

		/* consumed by the lisp machine when primitives return */
		lone_lisp_machine_push_primitive_delimiter(lone, machine);

		/* return the yielded value */
		lone_lisp_machine_push_value(lone, machine, value);
		return 0;

	default:
		break;
	}

	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(lone_intercept)
{
	struct lone_lisp_value arguments, clauses, body;

	switch (step) {
	case 0: /* decompose arguments, push data, evaluate body */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_is_nil(arguments)) {
			/* no arguments: (intercept) */ linux_exit(-1);
		}

		clauses = lone_lisp_list_first(lone, arguments);
		body = lone_lisp_list_rest(lone, arguments);

		if (lone_lisp_is_nil(body)) {
			/* no body: (intercept clauses) */ linux_exit(-1);
		}

		/* set up the stack so that signal can find
		 * the environment and clause list later
		 */
		lone_lisp_machine_push_value(lone, machine, machine->environment);
		lone_lisp_machine_push_value(lone, machine, clauses);
		lone_lisp_machine_push_interceptor_delimiter(lone, machine);

		/* evaluate the body as a sequence */
		machine->unevaluated = body;
		machine->step = LONE_LISP_MACHINE_STEP_SEQUENCE_EVALUATION_FROM_PRIMITIVE;
		return 1;

	case 1: /* evaluation completed, no error signalled */

		/* clean up the data that was meant for signal */
		lone_lisp_machine_pop_interceptor_delimiter(lone, machine);
		lone_lisp_machine_pop_value(lone, machine); /* clause list */
		lone_lisp_machine_pop_value(lone, machine); /* environment */

		/* return body's result */
		lone_lisp_machine_push_value(lone, machine, machine->value);
		return 0;

	default:
		break;
	}

	linux_exit(-1);
}

/* Scan the machine stack downward from the given position looking for
 * an interceptor delimiter. Returns a pointer to the delimiter frame
 * or null if none was found before hitting the stack base
 * or a generator delimiter. If a generator delimiter is found,
 * *generator is set to the generator heap struct.
 */
static struct lone_lisp_machine_stack_frame *
lone_lisp_signal_find_interceptor_delimiter_from(
		struct lone_lisp *lone, struct lone_lisp_machine *machine,
		struct lone_lisp_machine_stack_frame *start,
		struct lone_lisp_generator **generator)
{
	struct lone_lisp_machine_stack_frame *frame;

	*generator = 0;

	for (frame = start; frame >= machine->stack.base; --frame) {

		switch (frame->tagged & LONE_LISP_TAG_MASK) {
		case LONE_LISP_TAG_INTERCEPTOR_DELIMITER:
			return frame;
		case LONE_LISP_TAG_GENERATOR_DELIMITER:
			*generator = &lone_lisp_heap_value_of(
				lone,
				lone_lisp_retag_frame(*frame, LONE_LISP_TAG_GENERATOR)
			)->as.generator;
			return 0;
		default:
			continue;
		}
	}

	return 0;
}

static struct lone_lisp_machine_stack_frame *
lone_lisp_signal_find_interceptor_delimiter(
		struct lone_lisp *lone, struct lone_lisp_machine *machine,
		struct lone_lisp_generator **generator)
{
	return
		lone_lisp_signal_find_interceptor_delimiter_from(
			lone,
			machine,
			machine->stack.top - 1,
			generator
		);
}

/* Read clause list and environment relative to an interceptor delimiter.
 * Stack layout below delimiter:
 *
 * 	clauses-list
 * 	environment
 * 	...
 *
 */
static struct lone_lisp_value lone_lisp_signal_read_clause_list(
		struct lone_lisp_machine_stack_frame *delimiter)
{
	return (struct lone_lisp_value) { .tagged = (delimiter - 1)->tagged };
}

static struct lone_lisp_value lone_lisp_signal_read_environment(
		struct lone_lisp_machine_stack_frame *delimiter)
{
	return (struct lone_lisp_value) { .tagged = (delimiter - 2)->tagged };
}

static unsigned lone_lisp_function_arity(struct lone_lisp_value function)
{
	return (function.tagged >> LONE_LISP_METADATA_ARITY_SHIFT)
	                         & LONE_LISP_METADATA_ARITY_MASK;
}

/* Wrap a value in (quote value) so it survives evaluation.
 * Necessary when building expressions that the machine will evaluate,
 * because symbols would otherwise be looked up in the environment.
 */
static struct lone_lisp_value lone_lisp_signal_quote(struct lone_lisp *lone,
		struct lone_lisp_value value)
{
	struct lone_lisp_value quote = lone_lisp_intern_c_string(lone, "quote");
	return lone_lisp_list_build(lone, 2, &quote, &value);
}

/* Terminate a generator and propagate signal to the caller's stack.
 * Saves the current stack as the generator's own,
 * switches to the caller's stack and terminates the generator.
 * Then pushes a new function delimiter on the caller's stack
 * since the one pushed by the machine for this primitive
 * was on the old stack.
 */
static void lone_lisp_signal_terminate_generator(
		struct lone_lisp *lone,
		struct lone_lisp_machine *machine,
		struct lone_lisp_generator *generator)
{
	generator->stacks.own = machine->stack;
	machine->stack = generator->stacks.caller;
	machine->environment = lone_lisp_nil();
	generator->stacks.caller = (struct lone_lisp_machine_stack) { 0 };
	generator->stacks.own.top = 0;

	lone_lisp_machine_push_primitive_delimiter(lone, machine);
}

/* Push an interceptor's dispatch state onto the machine stack:
 * delimiter offset, environment and clause list.
 */
static void lone_lisp_signal_push_interceptor_state(
		struct lone_lisp *lone,
		struct lone_lisp_machine *machine,
		struct lone_lisp_machine_stack_frame *delimiter)
{
	lone_lisp_machine_push_integer(lone, machine,
		delimiter - machine->stack.base);

	lone_lisp_machine_push_value(lone, machine,
		lone_lisp_signal_read_environment(delimiter));

	lone_lisp_machine_push_value(lone, machine,
		lone_lisp_signal_read_clause_list(delimiter));
}

/* Signal dispatch stack layout (stable base, variable top):
 *
 * 	[variable]
 * 	intercept-environment
 * 	delimiter-offset
 * 	tag
 * 	value
 * 	...
 *
 * The bottom 4 frames are stable during clause iteration.
 * The [variable] part changes depending on the current step:
 *
 * 	evaluate_next_clause:
 *
 * 		handler-expression
 * 		clauses-list
 *
 * 	step 2, after match:
 *
 * 		handler-expression
 * 		clauses-list
 *
 * 	step 3, after predicate:
 *
 * 		handler-expression
 * 		clauses-list
 *
 *   step 4, after handler eval: nothing
 *
 * Tag and value are read by peeking at known stack depths
 * rather than popping and re-pushing the entire stack.
 */

LONE_LISP_PRIMITIVE(lone_signal)
{
	struct lone_lisp_machine_stack_frame *delimiter, *next, *frame, *frames;
	struct lone_lisp_value arguments, tag, value;
	struct lone_lisp_value clauses, clause, matcher, handler;
	struct lone_lisp_value matcher_expression, handler_expression;
	struct lone_lisp_value intercept_environment, continuation;
	struct lone_lisp_value quoted_tag, quoted_value;
	struct lone_lisp_generator *generator;
	long current_offset, delimiter_offset;
	size_t frame_count;
	unsigned arity;

	switch (step) {
	case 0: /* pop and destructure arguments, walk the stack */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(lone, arguments, 2, &tag, &value)) {
			/* wrong number of arguments */ linux_exit(-1);
		}

		if (lone_lisp_is_nil(tag)) {
			/* signal tag must not be nil */ linux_exit(-1);
		}

		/* find first interceptor delimiter */
		delimiter = lone_lisp_signal_find_interceptor_delimiter(
			lone,
			machine,
			&generator
		);

		while (!delimiter) {
			if (!generator) {
				/* no interceptor or generator delimiter found
				   signal cannot be handled */
				linux_exit(-1);
			}

			lone_lisp_signal_terminate_generator(lone, machine, generator);

			/* search for interceptor delimiter on the caller's stack */
			delimiter =
				lone_lisp_signal_find_interceptor_delimiter(
					lone,
					machine,
					&generator
				);

			/* loop continues */
		}

		/* push tag and value now that we have a delimiter to unwind to */
		lone_lisp_machine_push_value(lone, machine, value);
		lone_lisp_machine_push_value(lone, machine, tag);

		lone_lisp_signal_push_interceptor_state(lone, machine, delimiter);

	evaluate_next_clause:

		clauses = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_is_nil(clauses)) {
			/* no matching clauses, look for another interceptor */

			/* intercept_environment */
			lone_lisp_machine_pop_value(lone, machine);

			current_offset = lone_lisp_machine_pop_integer(lone, machine);

			/* search above the current delimiter's data */
			next =
				lone_lisp_signal_find_interceptor_delimiter_from(
					lone,
					machine,
					machine->stack.base + current_offset - 3,
					&generator
				);

			while (!next) {
				if (!generator) {
					/* no interceptor or generator delimiter found
					 signal cannot be handled */
					linux_exit(-1);
				}

				/* save tag and value before switching stacks */
				tag = lone_lisp_machine_pop_value(lone, machine);
				value = lone_lisp_machine_pop_value(lone, machine);

				lone_lisp_signal_terminate_generator(
					lone,
					machine,
					generator
				);

				/* restore tag and value on the new stack */
				lone_lisp_machine_push_value(lone, machine, value);
				lone_lisp_machine_push_value(lone, machine, tag);

				/* search on the caller's stack */
				next =
					lone_lisp_signal_find_interceptor_delimiter(
						lone,
						machine,
						&generator
					);
			}

			lone_lisp_signal_push_interceptor_state(lone, machine, next);

			goto evaluate_next_clause;
		}

		clause = lone_lisp_list_first(lone, clauses);
		clauses = lone_lisp_list_rest(lone, clauses);

		/* extract matcher and handler expressions from the clause */
		matcher_expression = lone_lisp_list_first(lone, clause);
		handler_expression =
			lone_lisp_list_first(lone, lone_lisp_list_rest(lone, clause));

		/* save remaining clauses and handler expression */
		lone_lisp_machine_push_value(lone, machine, clauses);
		lone_lisp_machine_push_value(lone, machine, handler_expression);

		/* intercept_environment is at a known and stable location */
		intercept_environment = lone_lisp_machine_peek_value(lone, machine, 3);

		/* request evaluation of matcher expression */
		machine->environment = intercept_environment;
		machine->expression = matcher_expression;
		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		return 2;

	case 2: /* matcher evaluated */

		/* Stack layout:
		 *
		 * 	handler_expression
		 * 	intercept_environment
		 * 	delimiter-offset
		 * 	tag
		 * 	value
		 * 	...
		 *
		 */

		handler_expression = lone_lisp_machine_pop_value(lone, machine);
		matcher = machine->value;

		if (lone_lisp_is_nil(matcher)) {
			/* nil matcher is the wildcard */
			goto evaluate_handler;
		}

		if (lone_lisp_is_function(lone, matcher)) {
			/* function matcher is a predicate, needs application */
			lone_lisp_machine_push_value(lone, machine, handler_expression);

			/* tag is at a known and stable location */
			tag = lone_lisp_machine_peek_value(lone, machine, 5);

			/* build and evaluate (predicate (quote tag)) */
			quoted_tag = lone_lisp_signal_quote(lone, tag);
			machine->expression = lone_lisp_list_build(
				lone,
				2,
				&matcher,
				&quoted_tag
			);

			machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
			return 3;
		}

		if (!lone_lisp_is_symbol(lone, matcher)) {
			/* matcher must be nil, function, or symbol */ linux_exit(-1);
		}

		/* symbol matcher, exact comparison */
		tag = lone_lisp_machine_peek_value(lone, machine, 4);

		if (lone_lisp_is_identical(lone, matcher, tag)) {
			goto evaluate_handler;
		}

		/* no match, try next clause */
		goto evaluate_next_clause;

	evaluate_handler:

		/* Stack layout:
		 *
		 * 	clauses-list
		 * 	intercept_environment
		 * 	delimiter-offset
		 * 	tag
		 * 	value
		 * 	...
		 *
		 */

		lone_lisp_machine_pop_value(lone, machine); /* clauses */
		intercept_environment = lone_lisp_machine_peek_value(lone, machine, 1);

		machine->environment = intercept_environment;
		machine->expression = handler_expression;
		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		return 4;

	case 3: /* predicate evaluated */

		/* Stack layout:
		 *
		 * 	handler-expression
		 * 	clauses-list
		 * 	intercept-environment
		 * 	delimiter-offset
		 * 	tag
		 * 	value
		 * 	...
		 *
		 */

		handler_expression = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_is_truthy(machine->value)) {
			goto evaluate_handler;
		}

		/* predicate did not match, try next clause */
		goto evaluate_next_clause;

	case 4: /* handler evaluated */

		/* Stack layout:
		 *
		 * 	intercept-environment
		 * 	delimiter-offset
		 * 	tag
		 * 	value
		 * 	...
		 *
		 */

		handler = machine->value;

		if (!lone_lisp_is_function(lone, handler)) {
			/* handler must be a lambda */ linux_exit(-1);
		}

		arity = lone_lisp_function_arity(handler);
		intercept_environment = lone_lisp_machine_pop_value(lone, machine);

		delimiter_offset = lone_lisp_machine_pop_integer(lone, machine);
		tag = lone_lisp_machine_pop_value(lone, machine);
		value = lone_lisp_machine_pop_value(lone, machine);

		delimiter = machine->stack.base + delimiter_offset;

		if (arity >= 2) {
			/* arity 2 handler: capture continuation then unwind */

			/* capture from the interceptor's associated data to stack top,
			 * excluding signal's own func-delim and save_step at the top */
			frame = delimiter - 2;
			frame_count = (machine->stack.top - 2) - frame;

			frames = lone_memory_array(
				lone->system,
				0,
				0,
				frame_count,
				sizeof(*frames),
				alignof(*frames)
			);

			lone_memory_move(
				frame,
				frames,
				lone_memory_array_size_in_bytes(
					frame_count,
					sizeof(*frames)
				)
			);

			continuation = lone_lisp_continuation_create(
				lone,
				frame_count,
				frames
			);

			machine->stack.top = frame;

		} else {
			/* arity 1 handler: just unwind past delimiter and its data */
			machine->stack.top = delimiter - 2;
		}

		/* build handler application
		 * value must be quoted,
		 * otherwise symbols will
		 * be dereferenced */
		quoted_value = lone_lisp_signal_quote(lone, value);
		if (arity >= 2) {
			machine->expression =
				lone_lisp_list_build(
					lone,
					3,
					&handler,
					&quoted_value,
					&continuation
				);
		} else {
			machine->expression =
				lone_lisp_list_build(
					lone,
					2,
					&handler,
					&quoted_value
				);
		}
		machine->step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		return 5;

	case 5: /* handler has returned */

		lone_lisp_machine_push_value(lone, machine, machine->value);
		return 0;

	default:
		break;
	}

	linux_exit(-1);
}

static bool lone_lisp_is_nil_predicate(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return lone_lisp_is_nil(value);
}

LONE_LISP_PRIMITIVE(lone_is_nil)
{
	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_apply_predicate(lone,
				lone_lisp_machine_pop_value(lone, machine), lone_lisp_is_nil_predicate));
	return 0;
}

static bool lone_lisp_is_true_predicate(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return lone_lisp_is_true(value);
}

LONE_LISP_PRIMITIVE(lone_is_true)
{
	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_apply_predicate(lone,
				lone_lisp_machine_pop_value(lone, machine), lone_lisp_is_true_predicate));
	return 0;
}

static bool lone_lisp_is_false_predicate(struct lone_lisp *lone, struct lone_lisp_value value)
{
	(void) lone;
	return lone_lisp_is_false(value);
}

LONE_LISP_PRIMITIVE(lone_is_false)
{
	lone_lisp_machine_push_value(lone, machine,
			lone_lisp_apply_predicate(lone,
				lone_lisp_machine_pop_value(lone, machine), lone_lisp_is_false_predicate));
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
		lone_lisp_print(lone, lone_lisp_list_first(lone, arguments), 1);
		linux_write(1, "\n", 1);
		arguments = lone_lisp_list_rest(lone, arguments);
	}

	lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
	return 0;
}
