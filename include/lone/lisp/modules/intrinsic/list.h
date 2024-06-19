/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_MODULES_INTRINSIC_LIST_HEADER
#define LONE_LISP_MODULES_INTRINSIC_LIST_HEADER

#include <lone/lisp/definitions.h>
#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    List operations.                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_lisp_modules_intrinsic_list_initialize(struct lone_lisp *lone);

LONE_LISP_PRIMITIVE(list_construct);
LONE_LISP_PRIMITIVE(list_first);
LONE_LISP_PRIMITIVE(list_rest);
LONE_LISP_PRIMITIVE(list_map);
LONE_LISP_PRIMITIVE(list_reduce);
LONE_LISP_PRIMITIVE(list_flatten);

#endif /* LONE_LISP_MODULES_INTRINSIC_LIST_HEADER */
