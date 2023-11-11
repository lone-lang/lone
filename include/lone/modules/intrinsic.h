/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MODULES_INTRINSIC_HEADER
#define LONE_MODULES_INTRINSIC_HEADER

#include <lone/types.h>

/* ╭─────────────────────┨ LONE / MODULES / INTRINSIC ┠─────────────────────╮
   │                                                                        │
   │    Initialization for built-in modules with essential functionality.   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_modules_intrinsic_initialize(
	struct lone_lisp *lone,
	int argument_count,
	char **argument_vector,
	char **environment,
	struct auxiliary *auxiliary_vector
);

#endif /* LONE_MODULES_INTRINSIC_HEADER */
