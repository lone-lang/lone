/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/modules.h>
#include <lone/modules/intrinsic/text.h>
#include <lone/utilities.h>
#include <lone/linux.h>

#include <lone/value/primitive.h>
#include <lone/value/list.h>
#include <lone/value/table.h>
#include <lone/value/symbol.h>
#include <lone/value/text.h>

void lone_modules_intrinsic_text_initialize(struct lone_lisp *lone)
{
	struct lone_value name, module, primitive;
	struct lone_function_flags flags;

	name = lone_intern_c_string(lone, "text");
	module = lone_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	primitive = lone_primitive_create(lone, "text_to_symbol", lone_primitive_text_to_symbol, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "to-symbol"), primitive);

	primitive = lone_primitive_create(lone, "join", lone_primitive_text_join, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "join"), primitive);

	primitive = lone_primitive_create(lone, "concatenate", lone_primitive_text_concatenate, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "concatenate"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

LONE_PRIMITIVE(text_to_symbol)
{
	struct lone_value text;

	if (lone_list_destructure(arguments, 1, &text)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	if (text.type != LONE_TYPE_HEAP_VALUE || text.as.heap_value->type != LONE_TYPE_TEXT) {
		/* argument not a text value: (to-symbol 123) */ linux_exit(-1);
	}

	return lone_text_to_symbol(lone, text);
}

LONE_PRIMITIVE(text_join)
{
	return lone_text_transfer_bytes(lone, lone_join(lone, lone_list_first(arguments), lone_list_rest(arguments), lone_is_text), true);
}

LONE_PRIMITIVE(text_concatenate)
{
	return lone_text_transfer_bytes(lone, lone_concatenate(lone, arguments, lone_is_text), true);
}
