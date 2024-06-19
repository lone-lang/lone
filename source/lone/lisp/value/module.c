/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/value.h>
#include <lone/lisp/value/module.h>
#include <lone/lisp/value/vector.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/heap.h>

struct lone_lisp_value lone_lisp_module_create(struct lone_lisp *lone, struct lone_lisp_value name)
{
	struct lone_lisp_heap_value *actual = lone_lisp_heap_allocate_value(lone);
	actual->type = LONE_LISP_TYPE_MODULE;
	actual->as.module.name = name;
	actual->as.module.environment = lone_lisp_table_create(lone, 64, lone->modules.top_level_environment);
	actual->as.module.exports = lone_lisp_vector_create(lone, 16);
	return lone_lisp_value_from_heap_value(actual);
}
