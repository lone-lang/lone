/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp.h>
#include <lone/memory.h>
#include <lone/hash.h>
#include <lone/modules.h>

#include <lone/value/module.h>
#include <lone/value/primitive.h>
#include <lone/value/list.h>
#include <lone/value/vector.h>
#include <lone/value/table.h>
#include <lone/value/symbol.h>

void lone_lisp_initialize(struct lone_lisp *lone, struct lone_bytes memory, void *stack, struct lone_bytes random)
{
	struct lone_function_flags flags = { .evaluate_arguments = 0, .evaluate_result = 0 };
	struct lone_value import, export;

	lone_memory_initialize(lone, memory, stack);

	lone_hash_initialize(lone, random);

	/* basic initialization done, can now use value creation functions */

	lone->symbol_table = lone_table_create(lone, 256, lone_nil());
	lone->constants.truth = lone_intern_c_string(lone, "true");

	lone->modules.loaded = lone_table_create(lone, 32, lone_nil());
	lone->modules.embedded = lone_nil();
	lone->modules.top_level_environment = lone_table_create(lone, 8, lone_nil());
	lone->modules.path = lone_vector_create(lone, 8);

	import = lone_primitive_create(lone, "import", lone_primitive_import, lone_nil(), flags);
	export = lone_primitive_create(lone, "export", lone_primitive_export, lone_nil(), flags);
	lone_table_set(lone, lone->modules.top_level_environment, lone_intern_c_string(lone, "import"), import);
	lone_table_set(lone, lone->modules.top_level_environment, lone_intern_c_string(lone, "export"), export);
	lone->modules.null = lone_module_create(lone, lone_nil());
}
