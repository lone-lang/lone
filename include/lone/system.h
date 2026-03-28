/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_SYSTEM_HEADER
#define LONE_SYSTEM_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The lone structure represents low level system state                │
   │    such as allocated memory and hash function state.                   │
   │    It must be initialized before everything else.                      │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_system_initialize(struct lone_system *system, struct lone_auxiliary_vector *auxiliary_vector, struct lone_bytes initial_static_memory);

#endif /* LONE_SYSTEM_HEADER */
