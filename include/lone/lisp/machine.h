/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_MACHINE_HEADER
#define LONE_LISP_MACHINE_HEADER

#include <lone/lisp/types.h>

void lone_lisp_machine_reset(struct lone_lisp *lone, struct lone_lisp_value module, struct lone_lisp_value expression);
bool lone_lisp_machine_cycle(struct lone_lisp *lone);

#endif /* LONE_LISP_MACHINE_HEADER */
