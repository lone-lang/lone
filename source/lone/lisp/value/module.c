/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>
#include <lone/lisp/heap.h>

struct lone_lisp_value lone_lisp_module_create(struct lone_lisp *lone, struct lone_lisp_value name)
{
	struct lone_lisp_heap_value *actual;
	struct lone_lisp_value environment, exports;

	/* create tables before allocating the module heap value
	   table_create transitively calls heap_allocate_value
	   which may grow the heap and invalidate pointers       */
	environment = lone_lisp_table_create(lone, 64, lone->modules.top_level_environment);
	exports     = lone_lisp_table_create(lone, 16, lone_lisp_nil());

	actual = lone_lisp_heap_allocate_value(lone);

	actual->as.module.name        = name;
	actual->as.module.environment = environment;
	actual->as.module.exports     = exports;

	return lone_lisp_value_from_heap_value(lone, actual, LONE_LISP_TAG_MODULE);
}
