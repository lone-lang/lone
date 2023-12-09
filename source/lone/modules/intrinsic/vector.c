/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/modules/intrinsic/vector.h>
#include <lone/modules.h>
#include <lone/value/primitive.h>
#include <lone/value/table.h>
#include <lone/value/list.h>
#include <lone/value/vector.h>
#include <lone/value/symbol.h>
#include <lone/lisp/evaluator.h>
#include <lone/linux.h>

void lone_modules_intrinsic_vector_initialize(struct lone_lisp *lone)
{
	struct lone_value name, module, primitive;
	struct lone_function_flags flags;

	name = lone_intern_c_string(lone, "vector");
	module = lone_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	primitive = lone_primitive_create(lone, "vector_get", lone_primitive_vector_get, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "get"), primitive);

	primitive = lone_primitive_create(lone, "vector_set", lone_primitive_vector_set, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "set"), primitive);

	primitive = lone_primitive_create(lone, "vector_slice", lone_primitive_vector_slice, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "slice"), primitive);

	primitive = lone_primitive_create(lone, "vector_each", lone_primitive_vector_each, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "each"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

LONE_PRIMITIVE(vector_get)
{
	struct lone_value vector, index;

	if (lone_list_destructure(arguments, 2, &vector, &index)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_is_vector(vector)) { /* vector not given: (get {}) */ linux_exit(-1); }
	if (!lone_is_integer(index)) { /* integer index not given: (get [1 2 3] "invalid") */ linux_exit(-1); }

	return lone_vector_get(lone, vector, index);
}

LONE_PRIMITIVE(vector_set)
{
	struct lone_value vector, index, value;

	if (lone_list_destructure(arguments, 3, &vector, &index, &value)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_is_vector(vector)) { /* vector not given: (set {}) */ linux_exit(-1); }
	if (!lone_is_integer(index)) { /* integer index not given: (set [1 2 3] "invalid") */ linux_exit(-1); }

	lone_vector_set(lone, vector, index, value);
	return lone_nil();
}

LONE_PRIMITIVE(vector_slice)
{
	struct lone_value vector, start, end, slice;
	struct lone_heap_value *actual;
	size_t i, j, k;

	if (lone_is_nil(arguments)) { /* arguments not given: (slice) */ linux_exit(-1); }

	vector = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_vector(vector)) { /* vector not given: (slice {}) */ linux_exit(-1); }
	if (lone_is_nil(arguments)) { /* start index not given: (slice vector) */ linux_exit(-1); }

	actual = vector.as.heap_value;

	start = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_integer(start)) { /* start is not an integer: (slice vector "error") */ linux_exit(-1); }

	i = start.as.unsigned_integer;

	if (lone_is_nil(arguments)) {
		j = actual->as.vector.count;
	} else {
		end = lone_list_first(arguments);
		arguments = lone_list_rest(arguments);
		if (!lone_is_nil(arguments)) { /* too many arguments given: (slice vector start end extra) */ linux_exit(-1); }
		if (!lone_is_integer(end)) { /* end is not an integer: (slice vector 10 "error") */ linux_exit(-1); }

		j = end.as.unsigned_integer;
	}

	slice = lone_vector_create(lone, j - i);

	for (k = 0; i < j; ++i, ++k) {
		lone_vector_set_value_at(lone, slice, k, lone_vector_get_value_at(lone, vector, i));
	}

	return slice;
}

LONE_PRIMITIVE(vector_each)
{
	struct lone_value result, vector, f, entry;
	size_t i;

	if (lone_list_destructure(arguments, 2, &vector, &f)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_is_vector(vector)) { /* vector not given: (each {}) */ linux_exit(-1); }
	if (!lone_is_applicable(f)) { /* applicable not given: (each vector []) */ linux_exit(-1); }

	result = lone_nil();

	LONE_VECTOR_FOR_EACH(entry, vector, i) {
		arguments = lone_list_build(lone, 1, &entry);
		result = lone_apply(lone, module, environment, f, arguments);
	}

	return result;
}
