/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MODULES_INTRINSIC_TEXT_HEADER
#define LONE_MODULES_INTRINSIC_TEXT_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Text operations.                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_modules_intrinsic_text_initialize(struct lone_lisp *lone);

LONE_PRIMITIVE(text_join);
LONE_PRIMITIVE(text_concatenate);

#endif /* LONE_MODULES_INTRINSIC_TEXT_HEADER */
