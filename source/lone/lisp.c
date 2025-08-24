/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp.h>
#include <lone/lisp/heap.h>
#include <lone/lisp/module.h>

#include <lone/lisp/value/module.h>
#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/vector.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/symbol.h>

#include <lone/memory/array.h>

void lone_lisp_initialize(struct lone_lisp *lone, struct lone_system *system, void *native_stack)
{
	struct lone_lisp_function_flags flags = { .evaluate_arguments = 0, .evaluate_result = 0 };
	struct lone_lisp_value import, export;

	lone->system = system;
	lone->native_stack = native_stack;

	lone_lisp_heap_initialize(lone);

	/* system, memory, stack and heap initialized
	 * can now use lisp value creation functions
	 */

	lone->machine.stack.base = lone_memory_array(lone->system, 0, 1000, sizeof(*lone->machine.stack.base));
	lone->machine.stack.limit = lone->machine.stack.base + 1000;

	lone->symbol_table = lone_lisp_table_create(lone, 256, lone_lisp_nil());

	lone->modules.loaded = lone_lisp_table_create(lone, 32, lone_lisp_nil());
	lone->modules.embedded = lone_lisp_nil();
	lone->modules.top_level_environment = lone_lisp_table_create(lone, 8, lone_lisp_nil());
	lone->modules.path = lone_lisp_vector_create(lone, 8);

	import = lone_lisp_primitive_create(lone, "import", lone_lisp_primitive_module_import, lone_lisp_nil(), flags);
	export = lone_lisp_primitive_create(lone, "export", lone_lisp_primitive_module_export, lone_lisp_nil(), flags);
	lone_lisp_table_set(lone, lone->modules.top_level_environment, lone_lisp_intern_c_string(lone, "import"), import);
	lone_lisp_table_set(lone, lone->modules.top_level_environment, lone_lisp_intern_c_string(lone, "export"), export);

	lone->modules.null = lone_lisp_module_create(lone, lone_lisp_nil());
}
