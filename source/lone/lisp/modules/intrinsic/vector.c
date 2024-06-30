/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/vector.h>

#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/vector.h>
#include <lone/lisp/value/symbol.h>
#include <lone/lisp/value/integer.h>

#include <lone/lisp/module.h>
#include <lone/lisp/evaluator.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_vector_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_value name, module, primitive;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "vector");
	module = lone_lisp_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	primitive = lone_lisp_primitive_create(lone, "vector_get", lone_lisp_primitive_vector_get, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "get", primitive);

	primitive = lone_lisp_primitive_create(lone, "vector_set", lone_lisp_primitive_vector_set, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "set", primitive);

	primitive = lone_lisp_primitive_create(lone, "vector_slice", lone_lisp_primitive_vector_slice, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "slice", primitive);

	primitive = lone_lisp_primitive_create(lone, "vector_each", lone_lisp_primitive_vector_each, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "each", primitive);

	primitive = lone_lisp_primitive_create(lone, "vector_count", lone_lisp_primitive_vector_count, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "count", primitive);

	lone_lisp_table_set(lone, lone->modules.loaded, name, module);
}

LONE_LISP_PRIMITIVE(vector_get)
{
	struct lone_lisp_value vector, index;

	if (lone_lisp_list_destructure(arguments, 2, &vector, &index)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_vector(vector)) { /* vector not given: (get {}) */ linux_exit(-1); }
	if (!lone_lisp_is_integer(index)) { /* integer index not given: (get [1 2 3] "invalid") */ linux_exit(-1); }

	return lone_lisp_vector_get(lone, vector, index);
}

LONE_LISP_PRIMITIVE(vector_set)
{
	struct lone_lisp_value vector, index, value;

	if (lone_lisp_list_destructure(arguments, 3, &vector, &index, &value)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_vector(vector)) { /* vector not given: (set {}) */ linux_exit(-1); }
	if (!lone_lisp_is_integer(index)) { /* integer index not given: (set [1 2 3] "invalid") */ linux_exit(-1); }

	lone_lisp_vector_set(lone, vector, index, value);
	return lone_lisp_nil();
}

LONE_LISP_PRIMITIVE(vector_slice)
{
	struct lone_lisp_value vector, start, end, slice;
	size_t i, j, k;

	if (lone_lisp_is_nil(arguments)) { /* arguments not given: (slice) */ linux_exit(-1); }

	vector = lone_lisp_list_first(arguments);
	arguments = lone_lisp_list_rest(arguments);
	if (!lone_lisp_is_vector(vector)) { /* vector not given: (slice {}) */ linux_exit(-1); }
	if (lone_lisp_is_nil(arguments)) { /* start index not given: (slice vector) */ linux_exit(-1); }

	start = lone_lisp_list_first(arguments);
	arguments = lone_lisp_list_rest(arguments);
	if (!lone_lisp_is_integer(start)) { /* start is not an integer: (slice vector "error") */ linux_exit(-1); }

	i = start.as.integer;

	if (lone_lisp_is_nil(arguments)) {
		j = lone_lisp_vector_count(vector);
	} else {
		end = lone_lisp_list_first(arguments);
		arguments = lone_lisp_list_rest(arguments);
		if (!lone_lisp_is_nil(arguments)) { /* too many arguments given: (slice vector start end extra) */ linux_exit(-1); }
		if (!lone_lisp_is_integer(end)) { /* end is not an integer: (slice vector 10 "error") */ linux_exit(-1); }

		j = end.as.integer;
	}

	slice = lone_lisp_vector_create(lone, j - i);

	for (k = 0; i < j; ++i, ++k) {
		lone_lisp_vector_set_value_at(lone, slice, k, lone_lisp_vector_get_value_at(vector, i));
	}

	return slice;
}

LONE_LISP_PRIMITIVE(vector_each)
{
	struct lone_lisp_value result, vector, f, entry;
	size_t i;

	if (lone_lisp_list_destructure(arguments, 2, &vector, &f)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_vector(vector)) { /* vector not given: (each {}) */ linux_exit(-1); }
	if (!lone_lisp_is_applicable(f)) { /* applicable not given: (each vector []) */ linux_exit(-1); }

	result = lone_lisp_nil();

	LONE_LISP_VECTOR_FOR_EACH(entry, vector, i) {
		arguments = lone_lisp_list_build(lone, 1, &entry);
		result = lone_lisp_apply(lone, module, environment, f, arguments);
	}

	return result;
}

LONE_LISP_PRIMITIVE(vector_count)
{
	struct lone_lisp_value vector;

	if (lone_lisp_list_destructure(arguments, 1, &vector)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_vector(vector)) { /* vector not given: (count {}) */ linux_exit(-1); }

	return lone_lisp_integer_create(lone_lisp_vector_count(vector));
}
