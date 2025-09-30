/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_VALUE_CONTINUATION_HEADER
#define LONE_LISP_VALUE_CONTINUATION_HEADER

#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone continuations reify segments of the lisp machine stack         │
   │    into a callable that can be used to resume the computation          │
   │    by plugging the argument into it.                                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_continuation_create(struct lone_lisp *lone,
		size_t frame_count, struct lone_lisp_machine_stack_frame *frames);

#endif /* LONE_LISP_VALUE_CONTINUATION_HEADER */
