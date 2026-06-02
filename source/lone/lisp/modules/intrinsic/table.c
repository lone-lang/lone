/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/table.h>
#include <lone/lisp/modules/intrinsic/lone.h>

#include <lone/lisp/machine.h>
#include <lone/lisp/machine/stack.h>
#include <lone/lisp/module.h>

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

	switch (step) {
	case 0:

		arguments = lone_lisp_machine_pop_value(lone, machine);

		goto destructure;

	case 1: /* resumed with replacement argument list */

		arguments = machine->value;

		if (!lone_lisp_is_list(lone, arguments)) {
			/* cannot destructure */
			return
				lone_lisp_signal_emit(
					lone,
					machine,
					1,
					lone->symbols.tags.type_error,
					arguments
				);
		}

		goto destructure;

	case 2: /* resumed with replacement table from type-error */

		key   = lone_lisp_machine_pop_value(lone, machine);
		table = machine->value;

		goto check_table;

	default:
		__builtin_trap();
	}

destructure:

	if (lone_lisp_list_destructure(lone, arguments, 2, &table, &key)) {
		/* wrong number of arguments: (get), (get {} 0 "extra") */
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				lone->symbols.tags.arity_error,
				arguments
			);
	}

check_table:

	if (!lone_lisp_is_table(lone, table)) {
		/* table not given: (get []) */
		lone_lisp_machine_push_value(lone, machine, key);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				2,
				lone->symbols.tags.type_error,
				table
			);
	}

	value = lone_lisp_table_get(lone, table, key);

	lone_lisp_machine_push_value(lone, machine, value);
	return 0;
}

LONE_LISP_PRIMITIVE(table_set)
{
	struct lone_lisp_value arguments, table, key, value;

	switch (step) {
	case 0:

		arguments = lone_lisp_machine_pop_value(lone, machine);

		goto destructure;

	case 1: /* resumed with replacement argument list */

		arguments = machine->value;

		if (!lone_lisp_is_list(lone, arguments)) {
			/* cannot destructure */
			return
				lone_lisp_signal_emit(
					lone,
					machine,
					1,
					lone->symbols.tags.type_error,
					arguments
				);
		}

		goto destructure;

	case 2: /* resumed with replacement table from type-error */

		value = lone_lisp_machine_pop_value(lone, machine);
		key   = lone_lisp_machine_pop_value(lone, machine);
		table = machine->value;

		goto check_table;

	default:
		__builtin_trap();
	}

destructure:

	if (lone_lisp_list_destructure(lone, arguments, 3, &table, &key, &value)) {
		/* wrong number of arguments: (set), (set {} 0 1 "extra") */
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				1,
				lone->symbols.tags.arity_error,
				arguments
			);
	}

check_table:

	if (!lone_lisp_is_table(lone, table)) {
		/* table not given: (set []) */
		lone_lisp_machine_push_value(lone, machine, key);
		lone_lisp_machine_push_value(lone, machine, value);
		return
			lone_lisp_signal_emit(
				lone,
				machine,
				2,
				lone->symbols.tags.type_error,
				table
			);
	}

	lone_lisp_table_set(lone, table, key, value);

	lone_lisp_machine_push_value(lone, machine, value);
	return 0;
}

LONE_LISP_PRIMITIVE(table_delete)
{
	struct lone_lisp_value arguments, table, key;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(lone, arguments, 2, &table, &key)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_table(lone, table)) { /* table not given: (delete []) */ linux_exit(-1); }

	lone_lisp_table_delete(lone, table, key);

	lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
	return 0;
}

LONE_LISP_PRIMITIVE(table_each)
{
	struct lone_lisp_value arguments, table, function, key, value;
	struct lone_lisp_heap_value *heap_value;
	struct lone_lisp_table_entry *entry;
	struct lone_lisp_shape *shape;
	lone_lisp_integer i;

	switch (step) {
	case 0: /* destructure arguments and initialize */

		arguments = lone_lisp_machine_pop_value(lone, machine);

		if (lone_lisp_list_destructure(lone, arguments, 2, &table, &function)) {
			/* wrong number of arguments */ linux_exit(-1);
		}

		if (!lone_lisp_is_table(lone, table)) { /* table not given: (each []) */ linux_exit(-1); }
		if (!lone_lisp_is_applicable(lone, function)) {
			/* applicable not given: (each table []) */ linux_exit(-1);
		}

		if (lone_lisp_heap_value_of(lone, table)->as.table.count < 1) {
			/* nothing to do */ break;
		}

		i = 0;

	advance:

		heap_value = lone_lisp_heap_value_of(lone, table);

		if (heap_value->shaped) {
			shape = &lone_lisp_heap_value_of(lone, heap_value->as.table.shaped.shape)->as.shape;

			if (i >= (lone_lisp_integer) shape->count) { break; }

			key   = shape->keys[i];
			value = heap_value->as.table.shaped.values[i];
		} else {
			while (   i < (lone_lisp_integer) heap_value->as.table.hash.used
			       && lone_lisp_is_tombstone(heap_value->as.table.hash.entries[i].key)) { ++i; }

			if (i >= (lone_lisp_integer) heap_value->as.table.hash.used) { break; }

			entry = &heap_value->as.table.hash.entries[i];
			key   = entry->key;
			value = entry->value;
		}

		machine->applicable = function;
		machine->list = lone_lisp_list_build(lone, 2, &key, &value);
		machine->step = LONE_LISP_MACHINE_STEP_APPLY;

		lone_lisp_machine_push_integer(lone, machine, i);
		lone_lisp_machine_push_value(lone, machine, function);
		lone_lisp_machine_push_value(lone, machine, table);

		return 1;

	case 1: /* advance or finish iteration */

		table    = lone_lisp_machine_pop_value(lone, machine);
		function = lone_lisp_machine_pop_value(lone, machine);
		i        = lone_lisp_machine_pop_integer(lone, machine);

		++i;

		goto advance;

	default:
		linux_exit(-1);
	}

	lone_lisp_machine_push_value(lone, machine, lone_lisp_nil());
	return 0;
}

LONE_LISP_PRIMITIVE(table_count)
{
	struct lone_lisp_value arguments, table, count;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(lone, arguments, 1, &table)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_table(lone, table)) { /* table not given: (count []) */ linux_exit(-1); }

	count = lone_lisp_integer_create(lone_lisp_table_count(lone, table));

	lone_lisp_machine_push_value(lone, machine, count);
	return 0;
}
