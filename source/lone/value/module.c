/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/value.h>
#include <lone/value/module.h>
#include <lone/value/vector.h>
#include <lone/value/table.h>

#include <lone/struct/lisp.h>
#include <lone/struct/value.h>
#include <lone/struct/module.h>

struct lone_value *lone_module_create(struct lone_lisp *lone, struct lone_value *name)
{
	struct lone_value *value = lone_value_create(lone);
	value->type = LONE_MODULE;
	value->module.name = name;
	value->module.environment = lone_table_create(lone, 64, lone->modules.top_level_environment);
	value->module.exports = lone_vector_create(lone, 16);
	return value;
}
