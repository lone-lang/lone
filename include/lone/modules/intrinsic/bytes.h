/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MODULES_INTRINSIC_BYTES_HEADER
#define LONE_MODULES_INTRINSIC_BYTES_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Bytes operations.                                                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_modules_intrinsic_bytes_initialize(struct lone_lisp *lone);

LONE_PRIMITIVE(bytes_new);

LONE_PRIMITIVE(bytes_read_u8);
LONE_PRIMITIVE(bytes_read_s8);
LONE_PRIMITIVE(bytes_read_u16);
LONE_PRIMITIVE(bytes_read_s16);
LONE_PRIMITIVE(bytes_read_u32);
LONE_PRIMITIVE(bytes_read_s32);

LONE_PRIMITIVE(bytes_write_u8);
LONE_PRIMITIVE(bytes_write_s8);
LONE_PRIMITIVE(bytes_write_u16);
LONE_PRIMITIVE(bytes_write_s16);
LONE_PRIMITIVE(bytes_write_u32);
LONE_PRIMITIVE(bytes_write_s32);

#endif /* LONE_MODULES_INTRINSIC_BYTES_HEADER */
