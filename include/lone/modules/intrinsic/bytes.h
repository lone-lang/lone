/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MODULES_INTRINSIC_BYTES_HEADER
#define LONE_MODULES_INTRINSIC_BYTES_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Bytes operations.                                                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_modules_intrinsic_bytes_initialize(struct lone_lisp *lone);

LONE_PRIMITIVE(bytes_new);

#endif /* LONE_MODULES_INTRINSIC_BYTES_HEADER */
