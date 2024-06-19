/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_HEADER
#define LONE_LISP_HEADER

#include <lone/types.h>
#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The lone lisp structure represents the lone lisp interpreter.       │
   │    A pointer to this structure is passed to nearly every function.     │
   │    It must be initialized before running lone lisp programs.           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_lisp_initialize(struct lone_lisp *lone, struct lone_system *system, void *native_stack);

#endif /* LONE_LISP_HEADER */
