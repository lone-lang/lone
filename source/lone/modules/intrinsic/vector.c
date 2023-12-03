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
	struct lone_value *name = lone_intern_c_string(lone, "vector"),
	                  *module = lone_module_for_name(lone, name),
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false, .variable_arguments = true };

	primitive = lone_primitive_create(lone, "vector_get", lone_primitive_vector_get, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "get"), primitive);

	primitive = lone_primitive_create(lone, "vector_set", lone_primitive_vector_set, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "set"), primitive);

	primitive = lone_primitive_create(lone, "vector_each", lone_primitive_vector_each, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "each"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

LONE_PRIMITIVE(vector_get)
{
	struct lone_value *vector, *index;

	if (lone_is_nil(arguments)) { /* arguments not given: (get) */ linux_exit(-1); }

	vector = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_vector(vector)) { /* vector not given: (get {}) */ linux_exit(-1); }
	if (lone_is_nil(arguments)) { /* index not given: (get vector) */ linux_exit(-1); }

	index = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* too many arguments given: (get vector index extra) */ linux_exit(-1); }

	return lone_vector_get(lone, vector, index);
}

LONE_PRIMITIVE(vector_set)
{
	struct lone_value *vector, *index, *value;

	if (lone_is_nil(arguments)) { /* arguments not given: (set) */ linux_exit(-1); }

	vector = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_vector(vector)) { /* vector not given: (set {}) */ linux_exit(-1); }
	if (lone_is_nil(arguments)) { /* index not given: (set vector) */ linux_exit(-1); }

	index = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(arguments)) { /* value not given: (set vector index) */ linux_exit(-1); }

	value = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* too many arguments given: (set vector index value extra) */ linux_exit(-1); }

	lone_vector_set(lone, vector, index, value);
	return lone_nil(lone);
}

LONE_PRIMITIVE(vector_each)
{
	struct lone_value *result, *vector, *f, *entry;
	size_t i;

	if (lone_is_nil(arguments)) { /* arguments not given: (each) */ linux_exit(-1); }

	vector = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_vector(vector)) { /* vector not given: (each {}) */ linux_exit(-1); }
	if (lone_is_nil(arguments)) { /* function not given: (each vector) */ linux_exit(-1); }

	f = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_applicable(f)) { /* applicable not given: (each vector []) */ linux_exit(-1); }
	if (!lone_is_nil(arguments)) { /* too many arguments given: (each vector applicable extra) */ linux_exit(-1); }

	LONE_VECTOR_FOR_EACH(entry, vector, i) {
		arguments = lone_list_build(lone, 1, entry);
		result = lone_apply(lone, module, environment, f, arguments);
	}

	return result;
}
