/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/value.h>
#include <lone/value/module.h>
#include <lone/value/vector.h>
#include <lone/value/table.h>
#include <lone/memory/heap.h>

struct lone_value lone_module_create(struct lone_lisp *lone, struct lone_value name)
{
	struct lone_heap_value *actual = lone_heap_allocate_value(lone);
	actual->type = LONE_MODULE;
	actual->as.module.name = name;
	actual->as.module.environment = lone_table_create(lone, 64, lone->modules.top_level_environment);
	actual->as.module.exports = lone_vector_create(lone, 16);
	return lone_value_from_heap_value(actual);
}
