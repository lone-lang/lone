/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_VALUE_POINTER_HEADER
#define LONE_LISP_VALUE_POINTER_HEADER

#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone pointers do not own the data they point to.                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_pointer_create(void *pointer);

#endif /* LONE_LISP_VALUE_POINTER_HEADER */
