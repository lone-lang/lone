/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/text.h>

#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/symbol.h>
#include <lone/lisp/value/text.h>

#include <lone/lisp/module.h>
#include <lone/lisp/utilities.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_text_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_value name, module, primitive;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "text");
	module = lone_lisp_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	primitive = lone_lisp_primitive_create(lone, "text_to_symbol", lone_lisp_primitive_text_to_symbol, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "to-symbol", primitive);

	primitive = lone_lisp_primitive_create(lone, "join", lone_lisp_primitive_text_join, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "join", primitive);

	primitive = lone_lisp_primitive_create(lone, "concatenate", lone_lisp_primitive_text_concatenate, module, flags);
	lone_lisp_module_set_and_export_c_string(lone, module, "concatenate", primitive);

	lone_lisp_table_set(lone, lone->modules.loaded, name, module);
}

LONE_LISP_PRIMITIVE(text_to_symbol)
{
	struct lone_lisp_value text;

	if (lone_lisp_list_destructure(arguments, 1, &text)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (text.type != LONE_LISP_TYPE_HEAP_VALUE || text.as.heap_value->type != LONE_LISP_TYPE_TEXT) {
		/* argument not a text value: (to-symbol 123) */ linux_exit(-1);
	}

	return lone_lisp_text_to_symbol(lone, text);
}

LONE_LISP_PRIMITIVE(text_join)
{
	struct lone_bytes joined;

	joined = lone_lisp_join(lone,
			lone_lisp_list_first(arguments),
			lone_lisp_list_rest(arguments),
			lone_lisp_is_text);

	return lone_lisp_text_transfer_bytes(lone, joined, true);
}

LONE_LISP_PRIMITIVE(text_concatenate)
{
	return lone_lisp_text_transfer_bytes(lone, lone_lisp_concatenate(lone, arguments, lone_lisp_is_text), true);
}
