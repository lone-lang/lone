/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MODULES_INTRINSIC_VECTOR_HEADER
#define LONE_MODULES_INTRINSIC_VECTOR_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Vector operations.                                                  │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_modules_intrinsic_vector_initialize(struct lone_lisp *lone);

LONE_PRIMITIVE(vector_get);
LONE_PRIMITIVE(vector_set);
LONE_PRIMITIVE(vector_slice);
LONE_PRIMITIVE(vector_each);
LONE_PRIMITIVE(vector_count);

#endif /* LONE_MODULES_INTRINSIC_VECTOR_HEADER */
