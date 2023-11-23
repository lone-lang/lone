/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/definitions.h>
#include <lone/types.h>

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

#include <lone/struct/lisp.h>

void lone_lisp_initialize(struct lone_lisp *lone, struct lone_bytes memory, size_t heap_size, void *stack, struct lone_bytes random)
{
	struct lone_function_flags flags = { .evaluate_arguments = 0, .evaluate_result = 0, .variable_arguments = 1 };
	struct lone_value *import, *export;

	lone_memory_initialize(lone, memory, heap_size, stack);

	lone_hash_initialize(lone, random);

	/* basic initialization done, can now use value creation functions */

	lone->symbol_table = lone_table_create(lone, 256, 0);
	lone->constants.nil = lone_list_create_nil(lone);
	lone->constants.truth = lone_intern_c_string(lone, "true");

	lone->modules.loaded = lone_table_create(lone, 32, 0);
	lone->modules.top_level_environment = lone_table_create(lone, 8, 0);
	lone->modules.path = lone_vector_create(lone, 8);

	import = lone_primitive_create(lone, "import", lone_primitive_import, 0, flags);
	export = lone_primitive_create(lone, "export", lone_primitive_export, 0, flags);
	lone_table_set(lone, lone->modules.top_level_environment, lone_intern_c_string(lone, "import"), import);
	lone_table_set(lone, lone->modules.top_level_environment, lone_intern_c_string(lone, "export"), export);
	lone->modules.null = lone_module_create(lone, 0);
}
