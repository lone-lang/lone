/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/modules/intrinsic/bytes.h>
#include <lone/modules.h>
#include <lone/memory/allocator.h>
#include <lone/linux.h>

#include <lone/value/primitive.h>
#include <lone/value/list.h>
#include <lone/value/table.h>
#include <lone/value/symbol.h>
#include <lone/value/bytes.h>

void lone_modules_intrinsic_bytes_initialize(struct lone_lisp *lone)
{
	struct lone_value name = lone_intern_c_string(lone, "bytes"),
	                  module = lone_module_for_name(lone, name),
	                  primitive;

	struct lone_function_flags flags = { .evaluate_arguments = true, .evaluate_result = false };

	primitive = lone_primitive_create(lone, "bytes_new", lone_primitive_bytes_new, module, flags);
	lone_set_and_export(lone, module, lone_intern_c_string(lone, "new"), primitive);

	lone_table_set(lone, lone->modules.loaded, name, module);
}

LONE_PRIMITIVE(bytes_new)
{
	struct lone_value count;
	size_t allocation;

	if (lone_list_destructure(arguments, 1, &count)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	switch (count.type) {
	case LONE_INTEGER:
		if (count.as.signed_integer <= 0) {
			/* zero or negative allocation, likely a mistake: (new 0), (new -64) */ linux_exit(-1);
		}

		allocation = count.as.unsigned_integer;
		break;
	case LONE_NIL:
	case LONE_POINTER:
	case LONE_HEAP_VALUE:
		/* count not an integer: (new {}) */ linux_exit(-1);
	}

	return lone_bytes_create(lone, allocation);
}
