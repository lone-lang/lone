/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/evaluator.h>
#include <lone/lisp/machine.h>

struct lone_lisp_value lone_lisp_evaluate_in_module(struct lone_lisp *lone,
		struct lone_lisp_value module, struct lone_lisp_value value)
{
	lone_lisp_machine_reset(lone, module, value);
	while (lone_lisp_machine_cycle(lone));
	return lone->machine.value;
}
