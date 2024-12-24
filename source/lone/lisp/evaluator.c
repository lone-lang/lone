/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/evaluator.h>

#include <lone/lisp/value/list.h>
#include <lone/lisp/value/vector.h>
#include <lone/lisp/value/table.h>

#include <lone/linux.h>

static struct lone_lisp_value lone_lisp_evaluate_form_index(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value environment,
		struct lone_lisp_value collection, struct lone_lisp_value arguments)
{
	struct lone_lisp_value (*get)(struct lone_lisp *, struct lone_lisp_value, struct lone_lisp_value);
	void (*set)(struct lone_lisp *, struct lone_lisp_value, struct lone_lisp_value, struct lone_lisp_value);
	struct lone_lisp_value key, value;

	switch (lone_lisp_value_to_type(collection)) {
	case LONE_LISP_TYPE_NIL:
	case LONE_LISP_TYPE_INTEGER:
	case LONE_LISP_TYPE_POINTER:
		linux_exit(-1);
	case LONE_LISP_TYPE_HEAP_VALUE:
		break;
	}

	switch (lone_lisp_value_to_heap_value(collection)->type) {
	case LONE_LISP_TYPE_VECTOR:
		get = lone_lisp_vector_get;
		set = lone_lisp_vector_set;
		break;
	case LONE_LISP_TYPE_TABLE:
		get = lone_lisp_table_get;
		set = lone_lisp_table_set;
		break;
	case LONE_LISP_TYPE_MODULE:
	case LONE_LISP_TYPE_FUNCTION:
	case LONE_LISP_TYPE_PRIMITIVE:
	case LONE_LISP_TYPE_BYTES:
	case LONE_LISP_TYPE_SYMBOL:
	case LONE_LISP_TYPE_TEXT:
	case LONE_LISP_TYPE_LIST:
		linux_exit(-1);
	}

	if (lone_lisp_is_nil(arguments)) { /* need at least the key: (collection) */ linux_exit(-1); }

	key = lone_lisp_list_first(arguments);
	arguments = lone_lisp_list_rest(arguments);

	if (lone_lisp_is_nil(arguments)) {
		/* collection get: (collection key) */
		key = lone_lisp_evaluate(lone, module, environment, key);

		return get(lone, collection, key);
	} else {
		/* at least one argument */
		value = lone_lisp_list_first(arguments);
		arguments = lone_lisp_list_rest(arguments);

		if (lone_lisp_is_nil(arguments)) {
			/* collection set: (collection key value) */
			key = lone_lisp_evaluate(lone, module, environment, key);
			value = lone_lisp_evaluate(lone, module, environment, value);

			set(lone, collection, key, value);

			return value;
		} else {
			/* too many arguments given: (collection key value extra) */
			linux_exit(-1);
		}
	}
}

static struct lone_lisp_value lone_lisp_evaluate_form(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value environment,
		struct lone_lisp_value list)
{
	struct lone_lisp_value first, rest;

	first = lone_lisp_list_first(list);
	first = lone_lisp_evaluate(lone, module, environment, first);

	switch (lone_lisp_value_to_type(first)) {
	case LONE_LISP_TYPE_NIL:
	case LONE_LISP_TYPE_INTEGER:
	case LONE_LISP_TYPE_POINTER:
		/* first element not applicable */ linux_exit(-1);
	case LONE_LISP_TYPE_HEAP_VALUE:
		break;
	}

	rest = lone_lisp_list_rest(list);

	/* apply arguments to the value */
	switch (lone_lisp_value_to_heap_value(first)->type) {
	case LONE_LISP_TYPE_FUNCTION:
	case LONE_LISP_TYPE_PRIMITIVE:
		return lone_lisp_apply(lone, module, environment, first, rest);
	case LONE_LISP_TYPE_VECTOR:
	case LONE_LISP_TYPE_TABLE:
		return lone_lisp_evaluate_form_index(lone, module, environment, first, rest);
	case LONE_LISP_TYPE_MODULE:
	case LONE_LISP_TYPE_LIST:
	case LONE_LISP_TYPE_SYMBOL:
	case LONE_LISP_TYPE_TEXT:
	case LONE_LISP_TYPE_BYTES:
		/* first element not an applicable type */ linux_exit(-1);
	}
}

struct lone_lisp_value lone_lisp_evaluate(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value environment,
		struct lone_lisp_value value)
{
	switch (lone_lisp_value_to_type(value)) {
	case LONE_LISP_TYPE_NIL:
	case LONE_LISP_TYPE_INTEGER:
	case LONE_LISP_TYPE_POINTER:
		return value;
	case LONE_LISP_TYPE_HEAP_VALUE:
		break;
	}

	switch (lone_lisp_value_to_heap_value(value)->type) {
	case LONE_LISP_TYPE_LIST:
		return lone_lisp_evaluate_form(lone, module, environment, value);
	case LONE_LISP_TYPE_SYMBOL:
		return lone_lisp_table_get(lone, environment, value);
	case LONE_LISP_TYPE_MODULE:
	case LONE_LISP_TYPE_FUNCTION:
	case LONE_LISP_TYPE_PRIMITIVE:
	case LONE_LISP_TYPE_VECTOR:
	case LONE_LISP_TYPE_TABLE:
	case LONE_LISP_TYPE_BYTES:
	case LONE_LISP_TYPE_TEXT:
		return value;
	}
}

struct lone_lisp_value lone_lisp_evaluate_all(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value environment,
		struct lone_lisp_value list)
{
	struct lone_lisp_value evaluated, head;

	for (evaluated = head = lone_lisp_nil(); !lone_lisp_is_nil(list); list = lone_lisp_list_rest(list)) {
		lone_lisp_list_append(lone, &evaluated, &head,
			lone_lisp_evaluate(lone, module, environment, lone_lisp_list_first(list)));
	}

	return evaluated;
}

struct lone_lisp_value lone_lisp_evaluate_in_module(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value value)
{
	return lone_lisp_evaluate(lone, module, lone_lisp_value_to_heap_value(module)->as.module.environment, value);
}

static struct lone_lisp_value lone_lisp_apply_function(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value environment,
		struct lone_lisp_value function, struct lone_lisp_value arguments)
{
	struct lone_lisp_value new_environment, names, code, value, current;

	names = lone_lisp_value_to_heap_value(function)->as.function.arguments;
	code = lone_lisp_value_to_heap_value(function)->as.function.code;
	value = lone_lisp_nil();

	new_environment = lone_lisp_table_create(lone, 16,
		lone_lisp_value_to_heap_value(function)->as.function.environment);

	/* evaluate each argument if function is configured to do so */
	if (lone_lisp_value_to_heap_value(function)->as.function.flags.evaluate_arguments) {
		arguments = lone_lisp_evaluate_all(lone, module, environment, arguments);
	}

	while (1) {
		if (!lone_lisp_is_nil(names)) {
			current = lone_lisp_list_first(names);

			switch (lone_lisp_value_to_type(current)) {
			case LONE_LISP_TYPE_HEAP_VALUE:
				break;
			case LONE_LISP_TYPE_NIL:
			case LONE_LISP_TYPE_POINTER:
			case LONE_LISP_TYPE_INTEGER:
				/* unexpected value */ linux_exit(-1);
			}

			switch (lone_lisp_value_to_heap_value(current)->type) {
			case LONE_LISP_TYPE_SYMBOL:
				/* normal argument passing: (lambda (x y)) */

				if (!lone_lisp_is_nil(arguments)) {
					/* argument matched to name, set name in environment */
					lone_lisp_table_set(lone, new_environment, current, lone_lisp_list_first(arguments));
				} else {
					/* argument number mismatch: ((lambda (x y) y) 10) */ linux_exit(-1);
				}

				break;
			case LONE_LISP_TYPE_LIST:
				/* variadic argument passing: (lambda ((arguments))), (lambda (x y (rest))) */

				if (!lone_lisp_is_symbol(lone_lisp_list_first(current))) {
					/* no name given: (lambda (x y ())) */ linux_exit(-1);
				} else if (lone_lisp_list_has_rest(current)) {
					/* too many names given: (lambda (x y (rest extra))) */ linux_exit(-1);
				} else {
					/* match list of remaining arguments to name */
					lone_lisp_table_set(lone, new_environment, lone_lisp_list_first(current), arguments);
					goto names_bound;
				}

			case LONE_LISP_TYPE_MODULE:
			case LONE_LISP_TYPE_FUNCTION:
			case LONE_LISP_TYPE_PRIMITIVE:
			case LONE_LISP_TYPE_BYTES:
			case LONE_LISP_TYPE_TEXT:
			case LONE_LISP_TYPE_VECTOR:
			case LONE_LISP_TYPE_TABLE:
				/* unexpected value */ linux_exit(-1);
			}

			names = lone_lisp_list_rest(names);
			arguments = lone_lisp_list_rest(arguments);

		} else if (!lone_lisp_is_nil(arguments)) {
			/* argument number mismatch: ((lambda (x) x) 10 20) */ linux_exit(-1);
		} else {
			/* end of function application with matching arguments */
			break;
		}
	}

names_bound:
	/* arguments have been bound to names in new environment */

	/* evaluate each lisp expression in function body */
	while (1) {
		if (lone_lisp_is_nil(code)) { break; }
		value = lone_lisp_list_first(code);
		value = lone_lisp_evaluate(lone, module, new_environment, value);
		code = lone_lisp_list_rest(code);
	}

	/* evaluate result if function is configured to do so */
	if (lone_lisp_value_to_heap_value(function)->as.function.flags.evaluate_result) {
		value = lone_lisp_evaluate(lone, module, environment, value);
	}

	return value;
}

static struct lone_lisp_value lone_lisp_apply_primitive(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value environment,
		struct lone_lisp_value primitive, struct lone_lisp_value arguments)
{
	struct lone_lisp_value result;

	if (lone_lisp_value_to_heap_value(primitive)->as.primitive.flags.evaluate_arguments) {
		arguments = lone_lisp_evaluate_all(lone, module, environment, arguments);
	}

	result = lone_lisp_value_to_heap_value(primitive)->as.primitive.function(
		lone,
		module,
		environment,
		arguments,
		lone_lisp_value_to_heap_value(primitive)->as.primitive.closure
	);

	if (lone_lisp_value_to_heap_value(primitive)->as.primitive.flags.evaluate_result) {
		result = lone_lisp_evaluate(lone, module, environment, result);
	}

	return result;
}

struct lone_lisp_value lone_lisp_apply(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value environment,
		struct lone_lisp_value applicable, struct lone_lisp_value arguments)
{
	if (!lone_lisp_is_applicable(applicable)) { /* given function is not an applicable type */ linux_exit(-1); }

	if (lone_lisp_is_function(applicable)) {
		return lone_lisp_apply_function(lone, module, environment, applicable, arguments);
	} else {
		return lone_lisp_apply_primitive(lone, module, environment, applicable, arguments);
	}
}
