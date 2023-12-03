/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/types.h>
#include <lone/lisp/evaluator.h>

#include <lone/value/list.h>
#include <lone/value/vector.h>
#include <lone/value/table.h>

#include <lone/linux.h>

static struct lone_value *lone_evaluate_form_index(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *collection, struct lone_value *arguments)
{
	struct lone_value *(*get)(struct lone_lisp *, struct lone_value *, struct lone_value *);
	void (*set)(struct lone_lisp *, struct lone_value *, struct lone_value *, struct lone_value *);
	struct lone_value *key, *value;

	switch (collection->type) {
	case LONE_VECTOR:
		get = lone_vector_get;
		set = lone_vector_set;
		break;
	case LONE_TABLE:
		get = lone_table_get;
		set = lone_table_set;
		break;
	case LONE_MODULE: case LONE_FUNCTION: case LONE_PRIMITIVE:
	case LONE_BYTES: case LONE_SYMBOL: case LONE_TEXT:
	case LONE_LIST: case LONE_INTEGER: case LONE_POINTER:
		linux_exit(-1);
	}

	if (lone_is_nil(arguments)) { /* need at least the key: (collection) */ linux_exit(-1); }
	key = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(arguments)) {
		/* table get: (collection key) */
		return get(lone, collection, lone_evaluate(lone, module, environment, key));
	} else {
		/* at least one argument */
		value = lone_list_first(arguments);
		arguments = lone_list_rest(arguments);
		if (lone_is_nil(arguments)) {
			/* table set: (collection key value) */
			set(lone, collection,
			          lone_evaluate(lone, module, environment, key),
			          lone_evaluate(lone, module, environment, value));
			return value;
		} else {
			/* too many arguments given: (collection key value extra) */
			linux_exit(-1);
		}
	}
}

static struct lone_value *lone_evaluate_form(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *list)
{
	struct lone_value *first = lone_list_first(list), *rest = lone_list_rest(list);

	/* apply arguments to a lone value */
	first = lone_evaluate(lone, module, environment, first);
	switch (first->type) {
	case LONE_FUNCTION:
	case LONE_PRIMITIVE:
		return lone_apply(lone, module, environment, first, rest);
	case LONE_VECTOR:
	case LONE_TABLE:
		return lone_evaluate_form_index(lone, module, environment, first, rest);
	case LONE_MODULE:
	case LONE_LIST:
	case LONE_SYMBOL:
	case LONE_TEXT:
	case LONE_BYTES:
	case LONE_INTEGER:
	case LONE_POINTER:
		/* first element not an applicable type */ linux_exit(-1);
	}
}

struct lone_value *lone_evaluate(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *value)
{
	if (value == 0) { return 0; }
	if (lone_is_nil(value)) { return value; }

	switch (value->type) {
	case LONE_LIST:
		return lone_evaluate_form(lone, module, environment, value);
	case LONE_SYMBOL:
		return lone_table_get(lone, environment, value);
	case LONE_MODULE:
	case LONE_FUNCTION:
	case LONE_PRIMITIVE:
	case LONE_VECTOR:
	case LONE_TABLE:
	case LONE_INTEGER:
	case LONE_POINTER:
	case LONE_BYTES:
	case LONE_TEXT:
		return value;
	}
}

struct lone_value *lone_evaluate_all(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *list)
{
	struct lone_value *evaluated = lone_list_create_nil(lone), *head;

	for (head = evaluated; !lone_is_nil(list); list = lone_list_rest(list)) {
		head = lone_list_append(lone, head, lone_evaluate(lone, module, environment, lone_list_first(list)));
	}

	return evaluated;
}

struct lone_value *lone_evaluate_module(struct lone_lisp *lone, struct lone_value *module, struct lone_value *value)
{
	return lone_evaluate(lone, module, module->module.environment, value);
}

static struct lone_value *lone_apply_function(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *function, struct lone_value *arguments)
{
	struct lone_value *new_environment = lone_table_create(lone, 16, function->function.environment),
	                  *names = function->function.arguments, *code = function->function.code,
	                  *value = lone_nil(lone);

	/* evaluate each argument if function is configured to do so */
	if (function->function.flags.evaluate_arguments) { arguments = lone_evaluate_all(lone, module, environment, arguments); }

	if (function->function.flags.variable_arguments) {
		if (lone_is_nil(names) || !lone_is_nil(lone_list_rest(names))) {
			/* must have exactly one argument: the list of arguments */
			linux_exit(-1);
		}

		lone_table_set(lone, new_environment, lone_list_first(names), arguments);
	} else {
		while (1) {
			if (lone_is_nil(names) != lone_is_nil(arguments)) {
				/* argument number mismatch: ((lambda (x) x) 10 20), ((lambda (x y) y) 10) */ linux_exit(-1);
			} else if (lone_is_nil(names) && lone_is_nil(arguments)) {
				/* end of function application with matching arguments */
				break;
			}

			/* valid binding, set name in environment and move on */

			lone_table_set(lone, new_environment, lone_list_first(names), lone_list_first(arguments));

			names = lone_list_rest(names);
			arguments = lone_list_rest(arguments);
		}
	}

	/* arguments have been bound to names in new environment */

	/* evaluate each lisp expression in function body */
	while (1) {
		if (lone_is_nil(code)) { break; }
		value = lone_list_first(code);
		value = lone_evaluate(lone, module, new_environment, value);
		code = lone_list_rest(code);
	}

	/* evaluate result if function is configured to do so */
	if (function->function.flags.evaluate_result) { value = lone_evaluate(lone, module, environment, value); }

	return value;
}

static struct lone_value *lone_apply_primitive(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *primitive, struct lone_value *arguments)
{
	struct lone_value *result;
	if (primitive->primitive.flags.evaluate_arguments) { arguments = lone_evaluate_all(lone, module, environment, arguments); }
	result = primitive->primitive.function(lone, module, environment, arguments, primitive->primitive.closure);
	if (primitive->primitive.flags.evaluate_result) { result = lone_evaluate(lone, module, environment, result); }
	return result;
}

struct lone_value *lone_apply(struct lone_lisp *lone, struct lone_value *module, struct lone_value *environment, struct lone_value *applicable, struct lone_value *arguments)
{
	if (!lone_is_applicable(applicable)) { /* given function is not an applicable type */ linux_exit(-1); }

	if (lone_is_function(applicable)) {
		return lone_apply_function(lone, module, environment, applicable, arguments);
	} else {
		return lone_apply_primitive(lone, module, environment, applicable, arguments);
	}
}
