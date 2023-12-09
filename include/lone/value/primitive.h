/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_VALUE_PRIMITIVE_HEADER
#define LONE_VALUE_PRIMITIVE_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Primitives are lone functions implemented in C.                     │
   │    They are always variadic and must check their arguments.            │
   │    All of them must follow the primitive function prototype.           │
   │    They also have closures which are pointers to arbitrary data.       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value lone_primitive_create(
	struct lone_lisp *lone,
	char *name,
	lone_primitive function,
	struct lone_value closure,
	struct lone_function_flags flags
);

#endif /* LONE_VALUE_PRIMITIVE_HEADER */
