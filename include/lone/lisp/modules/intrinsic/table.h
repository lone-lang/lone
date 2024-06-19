/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_MODULES_INTRINSIC_TABLE_HEADER
#define LONE_LISP_MODULES_INTRINSIC_TABLE_HEADER

#include <lone/lisp/definitions.h>
#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Table operations.                                                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_lisp_modules_intrinsic_table_initialize(struct lone_lisp *lone);

LONE_LISP_PRIMITIVE(table_get);
LONE_LISP_PRIMITIVE(table_set);
LONE_LISP_PRIMITIVE(table_delete);
LONE_LISP_PRIMITIVE(table_each);
LONE_LISP_PRIMITIVE(table_count);

#endif /* LONE_LISP_MODULES_INTRINSIC_TABLE_HEADER */
