/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_VALUE_POINTER_HEADER
#define LONE_VALUE_POINTER_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone pointers do not own the data they point to.                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value lone_pointer_create(void *pointer, enum lone_pointer_type type);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Pointer dereferencing functions.                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value lone_pointer_dereference(struct lone_value pointer);

#endif /* LONE_VALUE_POINTER_HEADER */
