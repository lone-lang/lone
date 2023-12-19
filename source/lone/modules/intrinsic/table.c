/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/modules/intrinsic/table.h>
#include <lone/modules.h>
#include <lone/value/primitive.h>
#include <lone/value/table.h>
#include <lone/value/list.h>
#include <lone/value/symbol.h>
#include <lone/value/integer.h>
#include <lone/lisp/constants.h>
#include <lone/lisp/evaluator.h>
#include <lone/linux.h>

void lone_modules_intrinsic_table_initialize(struct lone_lisp *lone)
{
	struct lone_value name, module, primitive;
	struct lone_function_flags flags;

	name = lone_intern_c_string(lone, "table");
	module = lone_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	primitive = lone_primitive_create(lone, "table_get", lone_primitive_table_get, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "get"), primitive);

	primitive = lone_primitive_create(lone, "table_set", lone_primitive_table_set, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "set"), primitive);

	primitive = lone_primitive_create(lone, "table_delete", lone_primitive_table_delete, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "delete"), primitive);

	primitive = lone_primitive_create(lone, "table_each", lone_primitive_table_each, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "each"), primitive);

	primitive = lone_primitive_create(lone, "table_count", lone_primitive_table_count, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "count"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);

}

LONE_PRIMITIVE(table_get)
{
	struct lone_value table, key;

	if (lone_list_destructure(arguments, 2, &table, &key)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_is_table(table)) { /* table not given: (get []) */ linux_exit(-1); }

	return lone_table_get(lone, table, key);
}

LONE_PRIMITIVE(table_set)
{
	struct lone_value table, key, value;

	if (lone_list_destructure(arguments, 3, &table, &key, &value)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_is_table(table)) { /* table not given: (set []) */ linux_exit(-1); }

	lone_table_set(lone, table, key, value);
	return lone_nil();
}

LONE_PRIMITIVE(table_delete)
{
	struct lone_value table, key;

	if (lone_list_destructure(arguments, 2, &table, &key)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_is_table(table)) { /* table not given: (delete []) */ linux_exit(-1); }

	lone_table_delete(lone, table, key);
	return lone_nil();
}

LONE_PRIMITIVE(table_each)
{
	struct lone_value result, table, f;
	struct lone_table_entry *entry;
	size_t i;

	if (lone_list_destructure(arguments, 2, &table, &f)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_is_table(table)) { /* table not given: (each []) */ linux_exit(-1); }
	if (!lone_is_applicable(f)) { /* applicable not given: (each table []) */ linux_exit(-1); }

	result = lone_nil();

	LONE_TABLE_FOR_EACH(entry, table, i) {
		arguments = lone_list_build(lone, 2, &entry->key, &entry->value);
		result = lone_apply(lone, module, environment, f, arguments);
	}

	return result;
}

LONE_PRIMITIVE(table_count)
{
	struct lone_value table;

	if (lone_list_destructure(arguments, 1, &table)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_is_table(table)) { /* table not given: (count []) */ linux_exit(-1); }

	return lone_integer_create(lone_table_count(table));
}
