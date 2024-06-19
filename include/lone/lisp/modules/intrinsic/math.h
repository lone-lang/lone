/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_MODULES_INTRINSIC_MATH_HEADER
#define LONE_LISP_MODULES_INTRINSIC_MATH_HEADER

#include <lone/lisp/definitions.h>
#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Built-in mathematical and numeric operations.                       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_lisp_modules_intrinsic_math_initialize(struct lone_lisp *lone);

LONE_LISP_PRIMITIVE(math_add);
LONE_LISP_PRIMITIVE(math_subtract);
LONE_LISP_PRIMITIVE(math_multiply);
LONE_LISP_PRIMITIVE(math_divide);
LONE_LISP_PRIMITIVE(math_is_less_than);
LONE_LISP_PRIMITIVE(math_is_less_than_or_equal_to);
LONE_LISP_PRIMITIVE(math_is_greater_than);
LONE_LISP_PRIMITIVE(math_is_greater_than_or_equal_to);
LONE_LISP_PRIMITIVE(math_sign);
LONE_LISP_PRIMITIVE(math_is_zero);
LONE_LISP_PRIMITIVE(math_is_positive);
LONE_LISP_PRIMITIVE(math_is_negative);

#endif /* LONE_LISP_MODULES_INTRINSIC_MATH_HEADER */
