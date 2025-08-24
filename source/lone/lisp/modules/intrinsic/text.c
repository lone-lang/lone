/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/text.h>

#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/symbol.h>
#include <lone/lisp/value/text.h>

#include <lone/lisp/machine.h>
#include <lone/lisp/module.h>
#include <lone/lisp/utilities.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_text_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_value name, module;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "text");
	module = lone_lisp_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	lone_lisp_module_export_primitive(lone, module, "to-symbol",
			"text_to_symbol", lone_lisp_primitive_text_to_symbol, module, flags);

	lone_lisp_module_export_primitive(lone, module, "join",
			"text_join", lone_lisp_primitive_text_join, module, flags);

	lone_lisp_module_export_primitive(lone, module, "concatenate",
			"text_concatenate", lone_lisp_primitive_text_concatenate, module, flags);
}

LONE_LISP_PRIMITIVE(text_to_symbol)
{
	struct lone_lisp_value arguments, text, symbol;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(arguments, 1, &text)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (!lone_lisp_is_text(text)) {
		/* argument not a text value: (to-symbol 123) */ linux_exit(-1);
	}

	symbol = lone_lisp_text_to_symbol(lone, text);

	lone_lisp_machine_push_value(lone, machine, symbol);
	return 0;
}

LONE_LISP_PRIMITIVE(text_join)
{
	struct lone_lisp_value arguments, text;
	struct lone_bytes joined;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	joined = lone_lisp_join(lone,
			lone_lisp_list_first(arguments),
			lone_lisp_list_rest(arguments),
			lone_lisp_is_text);

	text = lone_lisp_text_transfer_bytes(lone, joined, true);

	lone_lisp_machine_push_value(lone, machine, text);
	return 0;
}

LONE_LISP_PRIMITIVE(text_concatenate)
{
	struct lone_lisp_value arguments, text;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	text = lone_lisp_text_transfer_bytes(lone, lone_lisp_concatenate(lone, arguments, lone_lisp_is_text), true);

	lone_lisp_machine_push_value(lone, machine, text);
	return 0;
}
