/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_SYSTEM_HEADER
#define LONE_SYSTEM_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The lone system structure represents low level system state         │
   │    such as allocated memory and random bytes for hash keying.          │
   │    It must be initialized before everything else.                      │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_system_initialize(struct lone_system *system, struct lone_auxiliary_vector *auxiliary_vector);

#endif /* LONE_SYSTEM_HEADER */
