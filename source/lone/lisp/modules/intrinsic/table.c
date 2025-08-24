/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/table.h>

#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/symbol.h>
#include <lone/lisp/value/integer.h>

#include <lone/lisp/machine.h>
#include <lone/lisp/module.h>
#include <lone/lisp/evaluator.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_table_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_value name, module;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "table");
	module = lone_lisp_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	lone_lisp_module_export_primitive(lone, module, "get",
			"table_get", lone_lisp_primitive_table_get, module, flags);

	lone_lisp_module_export_primitive(lone, module, "set",
			"table_set", lone_lisp_primitive_table_set, module, flags);

	lone_lisp_module_export_primitive(lone, module, "delete",
			"table_delete", lone_lisp_primitive_table_delete, module, flags);

	lone_lisp_module_export_primitive(lone, module, "each",
			"table_each", lone_lisp_primitive_table_each, module, flags);

	lone_lisp_module_export_primitive(lone, module, "count",
			"table_count", lone_lisp_primitive_table_count, module, flags);
}

LONE_LISP_PRIMITIVE(table_get)
{
	struct lone_lisp_value arguments, table, key, value;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(arguments, 2, &table, &key)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_table(table)) { /* table not given: (get []) */ linux_exit(-1); }

	value = lone_lisp_table_get(lone, table, key);

	lone_lisp_machine_push_value(lone, machine, value);
	return 0;
}

LONE_LISP_PRIMITIVE(table_set)
{
	struct lone_lisp_value arguments, table, key, value;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(arguments, 3, &table, &key, &value)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_table(table)) { /* table not given: (set []) */ linux_exit(-1); }

	lone_lisp_table_set(lone, table, key, value);

	lone_lisp_machine_push_value(lone, machine, value);
	return 0;
}

LONE_LISP_PRIMITIVE(table_delete)
{
	struct lone_lisp_value arguments, table, key;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(arguments, 2, &table, &key)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_table(table)) { /* table not given: (delete []) */ linux_exit(-1); }

	lone_lisp_table_delete(lone, table, key);

	lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
	return 0;
}

LONE_LISP_PRIMITIVE(table_each)
{
	struct lone_lisp_value arguments, table, function, expression;
	struct lone_lisp_table_entry *entry;
	lone_lisp_integer i;

	switch (step) {
	case 0: /* destructure arguments and initialize */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(arguments, 2, &table, &function)) {
			/* wrong number of arguments */ linux_exit(-1);
		}

		if (!lone_lisp_is_table(table)) { /* table not given: (each []) */ linux_exit(-1); }
		if (!lone_lisp_is_applicable(function)) { /* applicable not given: (each table []) */ linux_exit(-1); }

		if (lone_lisp_heap_value_of(table)->as.table.count < 1) {
			/* nothing to do */ break;
		}

		i = 0;

	iteration:

		entry = &lone_lisp_heap_value_of(table)->as.table.entries[i];
		expression = lone_lisp_list_build(lone, 3, &function, &entry->key, &entry->value);

		lone->machine.step = LONE_LISP_MACHINE_STEP_EXPRESSION_EVALUATION;
		lone->machine.expression = expression;

		lone_lisp_machine_push_integer(lone, machine, i);
		lone_lisp_machine_push_value(lone, machine, function);
		lone_lisp_machine_push_value(lone, machine, table);

		return 1;

	case 1: /* advance or finish iteration */

		table    = lone_lisp_machine_pop_value(lone, machine);
		function = lone_lisp_machine_pop_value(lone, machine);
		i        = lone_lisp_machine_pop_integer(lone, machine);

		++i;

		if (i < lone_lisp_heap_value_of(table)->as.table.count) {
			goto iteration;
		} else {
			break;
		}
	}

	lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
	return 0;
}

LONE_LISP_PRIMITIVE(table_count)
{
	struct lone_lisp_value arguments, table, count;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(arguments, 1, &table)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_table(table)) { /* table not given: (count []) */ linux_exit(-1); }

	count = lone_lisp_integer_create(lone_lisp_table_count(table));

	lone_lisp_machine_push_value(lone, machine, count);
	return 0;
}
