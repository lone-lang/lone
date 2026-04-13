/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_MACHINE_HEADER
#define LONE_LISP_MACHINE_HEADER

#include <lone/lisp/types.h>

void lone_lisp_machine_initialize(struct lone_lisp_machine *machine, struct lone_lisp_machine_stack stack,
		size_t initial_stack_count);
void lone_lisp_machine_reset(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		struct lone_lisp_value module, struct lone_lisp_value expression);
bool lone_lisp_machine_cycle(struct lone_lisp *lone, struct lone_lisp_machine *machine);

#endif /* LONE_LISP_MACHINE_HEADER */
