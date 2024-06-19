/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_MODULES_INTRINSIC_TEXT_HEADER
#define LONE_LISP_MODULES_INTRINSIC_TEXT_HEADER

#include <lone/lisp/definitions.h>
#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Text operations.                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_lisp_modules_intrinsic_text_initialize(struct lone_lisp *lone);

LONE_LISP_PRIMITIVE(text_to_symbol);
LONE_LISP_PRIMITIVE(text_join);
LONE_LISP_PRIMITIVE(text_concatenate);

#endif /* LONE_LISP_MODULES_INTRINSIC_TEXT_HEADER */
