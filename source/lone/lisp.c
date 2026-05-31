/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp.h>
#include <lone/lisp/heap.h>
#include <lone/lisp/module.h>

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

	lone->symbol_table = lone_lisp_table_create(lone, 256, lone_lisp_nil());

	lone->modules.loaded = lone_lisp_table_create(lone, 32, lone_lisp_nil());
	lone->modules.embedded = lone_lisp_nil();
	lone->modules.top_level_environment = lone_lisp_table_create(lone, 8, lone_lisp_nil());
	lone->modules.path = lone_lisp_vector_create(lone, 8);
	lone->modules.signal_primitive = lone_lisp_nil();

	lone->symbols.tags.type_error             = lone_lisp_intern_c_string(lone, "type-error");
	lone->symbols.tags.arity_error            = lone_lisp_intern_c_string(lone, "arity-error");
	lone->symbols.tags.integer_overflow       = lone_lisp_intern_c_string(lone, "integer-overflow");
	lone->symbols.tags.division_by_zero       = lone_lisp_intern_c_string(lone, "division-by-zero");
	lone->symbols.tags.index_error            = lone_lisp_intern_c_string(lone, "index-error");
	lone->symbols.tags.range_error            = lone_lisp_intern_c_string(lone, "range-error");
	lone->symbols.tags.frozen_error           = lone_lisp_intern_c_string(lone, "frozen-error");
	lone->symbols.tags.invalid_unicode        = lone_lisp_intern_c_string(lone, "invalid-unicode");
	lone->symbols.tags.generator_exhausted    = lone_lisp_intern_c_string(lone, "generator-exhausted");
	lone->symbols.tags.generator_reentry      = lone_lisp_intern_c_string(lone, "generator-reentry");

	import = lone_lisp_primitive_create(lone, "import", lone_lisp_primitive_module_import, lone_lisp_nil(), flags);
	export = lone_lisp_primitive_create(lone, "export", lone_lisp_primitive_module_export, lone_lisp_nil(), flags);
	lone_lisp_table_set(lone, lone->modules.top_level_environment, lone_lisp_intern_c_string(lone, "import"), import);
	lone_lisp_table_set(lone, lone->modules.top_level_environment, lone_lisp_intern_c_string(lone, "export"), export);

	lone->modules.null = lone_lisp_module_create(lone, lone_lisp_nil());
}
