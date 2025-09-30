/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_MODULES_INTRINSIC_LONE_HEADER
#define LONE_LISP_MODULES_INTRINSIC_LONE_HEADER

#include <lone/lisp/definitions.h>
#include <lone/lisp/types.h>

/* ╭───────────────────┨ LONE LISP PRIMITIVE FUNCTIONS ┠────────────────────╮
   │                                                                        │
   │    Lone lisp functions implemented in C.                               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_lisp_modules_intrinsic_lone_initialize(struct lone_lisp *lone);

LONE_LISP_PRIMITIVE(lone_begin);
LONE_LISP_PRIMITIVE(lone_when);
LONE_LISP_PRIMITIVE(lone_unless);
LONE_LISP_PRIMITIVE(lone_if);
LONE_LISP_PRIMITIVE(lone_let);
LONE_LISP_PRIMITIVE(lone_set);
LONE_LISP_PRIMITIVE(lone_quote);
LONE_LISP_PRIMITIVE(lone_quasiquote);
LONE_LISP_PRIMITIVE(lone_lambda);
LONE_LISP_PRIMITIVE(lone_lambda_bang);
LONE_LISP_PRIMITIVE(lone_lambda_star);
LONE_LISP_PRIMITIVE(lone_return);
LONE_LISP_PRIMITIVE(lone_control);
LONE_LISP_PRIMITIVE(lone_is_list);
LONE_LISP_PRIMITIVE(lone_is_vector);
LONE_LISP_PRIMITIVE(lone_is_table);
LONE_LISP_PRIMITIVE(lone_is_symbol);
LONE_LISP_PRIMITIVE(lone_is_text);
LONE_LISP_PRIMITIVE(lone_is_integer);
LONE_LISP_PRIMITIVE(lone_is_identical);
LONE_LISP_PRIMITIVE(lone_is_equivalent);
LONE_LISP_PRIMITIVE(lone_is_equal);
LONE_LISP_PRIMITIVE(lone_print);

#endif /* LONE_LISP_MODULES_INTRINSIC_LONE_HEADER */
