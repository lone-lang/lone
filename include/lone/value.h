/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_VALUE_HEADER
#define LONE_VALUE_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    General constructor for lone heap values.                           │
   │    Meant for other constructors to use.                                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_value lone_value_from_heap_value(struct lone_heap_value *heap_value);

#endif /* LONE_VALUE_HEADER */
