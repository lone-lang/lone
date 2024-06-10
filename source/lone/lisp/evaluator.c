/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/evaluator.h>

#include <lone/value/list.h>
#include <lone/value/vector.h>
#include <lone/value/table.h>

#include <lone/linux.h>

static struct lone_value lone_evaluate_form_index(struct lone_lisp *lone, struct lone_value module, struct lone_value environment, struct lone_value collection, struct lone_value arguments)
{
	struct lone_value (*get)(struct lone_lisp *, struct lone_value, struct lone_value);
	void (*set)(struct lone_lisp *, struct lone_value, struct lone_value, struct lone_value);
	struct lone_value key, value;
	struct lone_heap_value *actual;

	switch (collection.type) {
	case LONE_TYPE_NIL:
	case LONE_TYPE_INTEGER:
	case LONE_TYPE_POINTER:
		linux_exit(-1);
	case LONE_TYPE_HEAP_VALUE:
		break;
	}

	actual = collection.as.heap_value;

	switch (actual->type) {
	case LONE_TYPE_VECTOR:
		get = lone_vector_get;
		set = lone_vector_set;
		break;
	case LONE_TYPE_TABLE:
		get = lone_table_get;
		set = lone_table_set;
		break;
	case LONE_TYPE_MODULE:
	case LONE_TYPE_FUNCTION:
	case LONE_TYPE_PRIMITIVE:
	case LONE_TYPE_BYTES:
	case LONE_TYPE_SYMBOL:
	case LONE_TYPE_TEXT:
	case LONE_TYPE_LIST:
		linux_exit(-1);
	}

	if (lone_is_nil(arguments)) { /* need at least the key: (collection) */ linux_exit(-1); }
	key = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(arguments)) {
		/* collection get: (collection key) */
		key = lone_evaluate(lone, module, environment, key);
		return get(lone, collection, key);
	} else {
		/* at least one argument */
		value = lone_list_first(arguments);
		arguments = lone_list_rest(arguments);
		if (lone_is_nil(arguments)) {
			/* collection set: (collection key value) */
			key = lone_evaluate(lone, module, environment, key);
			value = lone_evaluate(lone, module, environment, value);
			set(lone, collection, key, value);
			return value;
		} else {
			/* too many arguments given: (collection key value extra) */
			linux_exit(-1);
		}
	}
}

static struct lone_value lone_evaluate_form(struct lone_lisp *lone, struct lone_value module, struct lone_value environment, struct lone_value list)
{
	struct lone_value first, rest;
	struct lone_heap_value *actual;

	first = lone_list_first(list);
	first = lone_evaluate(lone, module, environment, first);

	switch (first.type) {
	case LONE_TYPE_NIL:
	case LONE_TYPE_INTEGER:
	case LONE_TYPE_POINTER:
		/* first element not applicable */ linux_exit(-1);
	case LONE_TYPE_HEAP_VALUE:
		break;
	}

	actual = first.as.heap_value;
	rest = lone_list_rest(list);

	/* apply arguments to the value */
	switch (actual->type) {
	case LONE_TYPE_FUNCTION:
	case LONE_TYPE_PRIMITIVE:
		return lone_apply(lone, module, environment, first, rest);
	case LONE_TYPE_VECTOR:
	case LONE_TYPE_TABLE:
		return lone_evaluate_form_index(lone, module, environment, first, rest);
	case LONE_TYPE_MODULE:
	case LONE_TYPE_LIST:
	case LONE_TYPE_SYMBOL:
	case LONE_TYPE_TEXT:
	case LONE_TYPE_BYTES:
		/* first element not an applicable type */ linux_exit(-1);
	}
}

struct lone_value lone_evaluate(struct lone_lisp *lone, struct lone_value module, struct lone_value environment, struct lone_value value)
{
	struct lone_heap_value *actual;

	switch (value.type) {
	case LONE_TYPE_NIL:
	case LONE_TYPE_INTEGER:
	case LONE_TYPE_POINTER:
		return value;
	case LONE_TYPE_HEAP_VALUE:
		break;
	}

	actual = value.as.heap_value;

	switch (actual->type) {
	case LONE_TYPE_LIST:
		return lone_evaluate_form(lone, module, environment, value);
	case LONE_TYPE_SYMBOL:
		return lone_table_get(lone, environment, value);
	case LONE_TYPE_MODULE:
	case LONE_TYPE_FUNCTION:
	case LONE_TYPE_PRIMITIVE:
	case LONE_TYPE_VECTOR:
	case LONE_TYPE_TABLE:
	case LONE_TYPE_BYTES:
	case LONE_TYPE_TEXT:
		return value;
	}
}

struct lone_value lone_evaluate_all(struct lone_lisp *lone, struct lone_value module, struct lone_value environment, struct lone_value list)
{
	struct lone_value evaluated, head;

	for (evaluated = head = lone_nil(); !lone_is_nil(list); list = lone_list_rest(list)) {
		lone_list_append(lone, &evaluated, &head, lone_evaluate(lone, module, environment, lone_list_first(list)));
	}

	return evaluated;
}

struct lone_value lone_evaluate_in_module(struct lone_lisp *lone, struct lone_value module, struct lone_value value)
{
	return lone_evaluate(lone, module, module.as.heap_value->as.module.environment, value);
}

static struct lone_value lone_apply_function(struct lone_lisp *lone, struct lone_value module, struct lone_value environment, struct lone_value function, struct lone_value arguments)
{
	struct lone_value new_environment, names, code, value, current;
	struct lone_heap_value *actual;

	actual = function.as.heap_value;
	new_environment = lone_table_create(lone, 16, actual->as.function.environment);
	names = actual->as.function.arguments;
	code = actual->as.function.code;
	value = lone_nil();

	/* evaluate each argument if function is configured to do so */
	if (actual->as.function.flags.evaluate_arguments) { arguments = lone_evaluate_all(lone, module, environment, arguments); }

	while (1) {
		if (!lone_is_nil(names)) {
			current = lone_list_first(names);

			switch (current.type) {
			case LONE_TYPE_HEAP_VALUE:
				break;
			case LONE_TYPE_NIL:
			case LONE_TYPE_POINTER:
			case LONE_TYPE_INTEGER:
				/* unexpected value */ linux_exit(-1);
			}

			switch (current.as.heap_value->type) {
			case LONE_TYPE_SYMBOL:
				/* normal argument passing: (lambda (x y)) */

				if (!lone_is_nil(arguments)) {
					/* argument matched to name, set name in environment */
					lone_table_set(lone, new_environment, current, lone_list_first(arguments));
				} else {
					/* argument number mismatch: ((lambda (x y) y) 10) */ linux_exit(-1);
				}

				break;
			case LONE_TYPE_LIST:
				/* variadic argument passing: (lambda ((arguments))), (lambda (x y (rest))) */

				if (!lone_is_symbol(lone_list_first(current))) {
					/* no name given: (lambda (x y ())) */ linux_exit(-1);
				} else if (lone_list_has_rest(current)) {
					/* too many names given: (lambda (x y (rest extra))) */ linux_exit(-1);
				} else {
					/* match list of remaining arguments to name */
					lone_table_set(lone, new_environment, lone_list_first(current), arguments);
					goto names_bound;
				}

			case LONE_TYPE_MODULE:
			case LONE_TYPE_FUNCTION:
			case LONE_TYPE_PRIMITIVE:
			case LONE_TYPE_BYTES:
			case LONE_TYPE_TEXT:
			case LONE_TYPE_VECTOR:
			case LONE_TYPE_TABLE:
				/* unexpected value */ linux_exit(-1);
			}

			names = lone_list_rest(names);
			arguments = lone_list_rest(arguments);

		} else if (!lone_is_nil(arguments)) {
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
		if (lone_is_nil(code)) { break; }
		value = lone_list_first(code);
		value = lone_evaluate(lone, module, new_environment, value);
		code = lone_list_rest(code);
	}

	/* evaluate result if function is configured to do so */
	if (actual->as.function.flags.evaluate_result) { value = lone_evaluate(lone, module, environment, value); }

	return value;
}

static struct lone_value lone_apply_primitive(struct lone_lisp *lone, struct lone_value module, struct lone_value environment, struct lone_value primitive, struct lone_value arguments)
{
	struct lone_heap_value *actual = primitive.as.heap_value;
	struct lone_value result;

	if (actual->as.primitive.flags.evaluate_arguments) { arguments = lone_evaluate_all(lone, module, environment, arguments); }

	result = actual->as.primitive.function(lone, module, environment, arguments, actual->as.primitive.closure);

	if (actual->as.primitive.flags.evaluate_result) { result = lone_evaluate(lone, module, environment, result); }

	return result;
}

struct lone_value lone_apply(struct lone_lisp *lone, struct lone_value module, struct lone_value environment, struct lone_value applicable, struct lone_value arguments)
{
	if (!lone_is_applicable(applicable)) { /* given function is not an applicable type */ linux_exit(-1); }

	if (lone_is_function(applicable)) {
		return lone_apply_function(lone, module, environment, applicable, arguments);
	} else {
		return lone_apply_primitive(lone, module, environment, applicable, arguments);
	}
}
