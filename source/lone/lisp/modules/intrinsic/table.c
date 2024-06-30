/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/table.h>

#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/symbol.h>
#include <lone/lisp/value/integer.h>

#include <lone/lisp/module.h>
#include <lone/lisp/constants.h>
#include <lone/lisp/evaluator.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_table_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_value name, module, primitive;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "table");
	module = lone_lisp_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	primitive = lone_lisp_primitive_create(lone, "table_get", lone_lisp_primitive_table_get, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "get", primitive);

	primitive = lone_lisp_primitive_create(lone, "table_set", lone_lisp_primitive_table_set, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "set", primitive);

	primitive = lone_lisp_primitive_create(lone, "table_delete", lone_lisp_primitive_table_delete, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "delete", primitive);

	primitive = lone_lisp_primitive_create(lone, "table_each", lone_lisp_primitive_table_each, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "each", primitive);

	primitive = lone_lisp_primitive_create(lone, "table_count", lone_lisp_primitive_table_count, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "count", primitive);
}

LONE_LISP_PRIMITIVE(table_get)
{
	struct lone_lisp_value table, key;

	if (lone_lisp_list_destructure(arguments, 2, &table, &key)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_table(table)) { /* table not given: (get []) */ linux_exit(-1); }

	return lone_lisp_table_get(lone, table, key);
}

LONE_LISP_PRIMITIVE(table_set)
{
	struct lone_lisp_value table, key, value;

	if (lone_lisp_list_destructure(arguments, 3, &table, &key, &value)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_table(table)) { /* table not given: (set []) */ linux_exit(-1); }

	lone_lisp_table_set(lone, table, key, value);
	return lone_lisp_nil();
}

LONE_LISP_PRIMITIVE(table_delete)
{
	struct lone_lisp_value table, key;

	if (lone_lisp_list_destructure(arguments, 2, &table, &key)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_table(table)) { /* table not given: (delete []) */ linux_exit(-1); }

	lone_lisp_table_delete(lone, table, key);
	return lone_lisp_nil();
}

LONE_LISP_PRIMITIVE(table_each)
{
	struct lone_lisp_value result, table, f;
	struct lone_lisp_table_entry *entry;
	size_t i;

	if (lone_lisp_list_destructure(arguments, 2, &table, &f)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_table(table)) { /* table not given: (each []) */ linux_exit(-1); }
	if (!lone_lisp_is_applicable(f)) { /* applicable not given: (each table []) */ linux_exit(-1); }

	result = lone_lisp_nil();

	LONE_LISP_TABLE_FOR_EACH(entry, table, i) {
		arguments = lone_lisp_list_build(lone, 2, &entry->key, &entry->value);
		result = lone_lisp_apply(lone, module, environment, f, arguments);
	}

	return result;
}

LONE_LISP_PRIMITIVE(table_count)
{
	struct lone_lisp_value table;

	if (lone_lisp_list_destructure(arguments, 1, &table)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_table(table)) { /* table not given: (count []) */ linux_exit(-1); }

	return lone_lisp_integer_create(lone_lisp_table_count(table));
}
