/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_VALUE_HEADER
#define LONE_LISP_VALUE_HEADER

#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    General constructor for lone heap values.                           │
   │    Meant for other constructors to use.                                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_value_from_heap_value(struct lone_lisp_heap_value *heap_value);

#endif /* LONE_LISP_VALUE_HEADER */
