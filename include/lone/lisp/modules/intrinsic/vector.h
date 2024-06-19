/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_MODULES_INTRINSIC_VECTOR_HEADER
#define LONE_LISP_MODULES_INTRINSIC_VECTOR_HEADER

#include <lone/lisp/definitions.h>
#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Vector operations.                                                  │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_lisp_modules_intrinsic_vector_initialize(struct lone_lisp *lone);

LONE_LISP_PRIMITIVE(vector_get);
LONE_LISP_PRIMITIVE(vector_set);
LONE_LISP_PRIMITIVE(vector_slice);
LONE_LISP_PRIMITIVE(vector_each);
LONE_LISP_PRIMITIVE(vector_count);

#endif /* LONE_LISP_MODULES_INTRINSIC_VECTOR_HEADER */
