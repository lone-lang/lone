/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_VALUE_PRIMITIVE_HEADER
#define LONE_LISP_VALUE_PRIMITIVE_HEADER

#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Primitives are lone functions implemented in C.                     │
   │    They are always variadic and must check their arguments.            │
   │    All of them must follow the primitive function prototype.           │
   │    They also have closures which are pointers to arbitrary data.       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_primitive_create(struct lone_lisp *lone, char *name,
		lone_lisp_primitive_function function, struct lone_lisp_value closure,
		struct lone_lisp_function_flags flags);

#endif /* LONE_LISP_VALUE_PRIMITIVE_HEADER */
