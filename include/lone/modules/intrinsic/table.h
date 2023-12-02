/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MODULES_INTRINSIC_TABLE_HEADER
#define LONE_MODULES_INTRINSIC_TABLE_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Table operations.                                                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_modules_intrinsic_table_initialize(struct lone_lisp *lone);

LONE_PRIMITIVE(table_get);
LONE_PRIMITIVE(table_set);
LONE_PRIMITIVE(table_delete);
LONE_PRIMITIVE(table_each);

#endif /* LONE_MODULES_INTRINSIC_TABLE_HEADER */
