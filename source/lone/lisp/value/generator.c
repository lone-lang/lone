/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>
#include <lone/lisp/heap.h>
#include <lone/lisp/machine.h>

struct lone_lisp_value lone_lisp_generator_create(struct lone_lisp *lone,
		struct lone_lisp_value function, size_t stack_size)
{
	struct lone_lisp_heap_value *actual = lone_lisp_heap_allocate_value(lone);
	actual->as.generator.function = function;
	actual->as.generator.stacks.own = lone_lisp_machine_allocate_stack(lone, stack_size);
	actual->as.generator.stacks.caller = (struct lone_lisp_machine_stack) { 0 };
	return lone_lisp_value_from_heap_value(lone, actual, LONE_LISP_TAG_GENERATOR);
}
