/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MODULES_MATH_HEADER
#define LONE_MODULES_MATH_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Built-in mathematical and numeric operations.                       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_module_math_initialize(struct lone_lisp *lone);

LONE_PRIMITIVE(math_add);
LONE_PRIMITIVE(math_subtract);
LONE_PRIMITIVE(math_multiply);
LONE_PRIMITIVE(math_divide);
LONE_PRIMITIVE(math_is_less_than);
LONE_PRIMITIVE(math_is_less_than_or_equal_to);
LONE_PRIMITIVE(math_is_greater_than);
LONE_PRIMITIVE(math_is_greater_than_or_equal_to);
LONE_PRIMITIVE(math_sign);
LONE_PRIMITIVE(math_is_zero);
LONE_PRIMITIVE(math_is_positive);
LONE_PRIMITIVE(math_is_negative);

#endif /* LONE_MODULES_MATH_HEADER */
