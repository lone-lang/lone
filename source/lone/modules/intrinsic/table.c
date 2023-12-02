/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/modules/intrinsic/table.h>
#include <lone/modules.h>
#include <lone/value/primitive.h>
#include <lone/value/table.h>
#include <lone/value/list.h>
#include <lone/value/symbol.h>
#include <lone/lisp/constants.h>
#include <lone/lisp/evaluator.h>
#include <lone/linux.h>

void lone_modules_intrinsic_table_initialize(struct lone_lisp *lone)
{
	struct lone_value *name = lone_intern_c_string(lone, "table"),
	                  *module = lone_module_for_name(lone, name),
	                  *primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false, .variable_arguments = true };

	primitive = lone_primitive_create(lone, "table_get", lone_primitive_table_get, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "get"), primitive);

	primitive = lone_primitive_create(lone, "table_set", lone_primitive_table_set, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "set"), primitive);

	primitive = lone_primitive_create(lone, "table_delete", lone_primitive_table_delete, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "delete"), primitive);

	primitive = lone_primitive_create(lone, "table_each", lone_primitive_table_each, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "each"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);

}

LONE_PRIMITIVE(table_get)
{
	struct lone_value *table, *key;

	if (lone_is_nil(arguments)) { /* arguments not given: (get) */ linux_exit(-1); }

	table = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_table(table)) { /* table not given: (get []) */ linux_exit(-1); }
	if (lone_is_nil(arguments)) { /* key not given: (get table) */ linux_exit(-1); }

	key = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* too many arguments given: (get table key extra) */ linux_exit(-1); }

	return lone_table_get(lone, table, key);
}

LONE_PRIMITIVE(table_set)
{
	struct lone_value *table, *key, *value;

	if (lone_is_nil(arguments)) { /* arguments not given: (set) */ linux_exit(-1); }

	table = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_table(table)) { /* table not given: (set []) */ linux_exit(-1); }
	if (lone_is_nil(arguments)) { /* key not given: (set table) */ linux_exit(-1); }

	key = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (lone_is_nil(arguments)) { /* value not given: (set table key) */ linux_exit(-1); }

	value = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* too many arguments given: (set table key value extra) */ linux_exit(-1); }

	lone_table_set(lone, table, key, value);
	return lone_nil(lone);
}

LONE_PRIMITIVE(table_delete)
{
	struct lone_value *table, *key;

	if (lone_is_nil(arguments)) { /* arguments not given: (delete) */ linux_exit(-1); }

	table = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_table(table)) { /* table not given: (delete []) */ linux_exit(-1); }
	if (lone_is_nil(arguments)) { /* key not given: (delete table) */ linux_exit(-1); }

	key = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_nil(arguments)) { /* too many arguments given: (delete table key extra) */ linux_exit(-1); }

	lone_table_delete(lone, table, key);
	return lone_nil(lone);
}

LONE_PRIMITIVE(table_each)
{
	struct lone_value *result, *table, *f;
	struct lone_table_entry *entry;
	size_t i;

	if (lone_is_nil(arguments)) { /* arguments not given: (each) */ linux_exit(-1); }

	table = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_table(table)) { /* table not given: (each []) */ linux_exit(-1); }
	if (lone_is_nil(arguments)) { /* function not given: (each table) */ linux_exit(-1); }

	f = lone_list_first(arguments);
	arguments = lone_list_rest(arguments);
	if (!lone_is_applicable(f)) { /* applicable not given: (each table []) */ linux_exit(-1); }
	if (!lone_is_nil(arguments)) { /* too many arguments given: (each table applicable extra) */ linux_exit(-1); }

	LONE_TABLE_FOR_EACH(entry, table, i) {
		arguments = lone_list_build(lone, 2, entry->key, entry->value);
		result = lone_apply(lone, module, environment, f, arguments);
	}

	return result;
}
