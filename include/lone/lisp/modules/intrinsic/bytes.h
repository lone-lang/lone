/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_MODULES_INTRINSIC_BYTES_HEADER
#define LONE_LISP_MODULES_INTRINSIC_BYTES_HEADER

#include <lone/lisp/definitions.h>
#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Bytes operations.                                                   │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_lisp_modules_intrinsic_bytes_initialize(struct lone_lisp *lone);

LONE_LISP_PRIMITIVE(bytes_new);
LONE_LISP_PRIMITIVE(bytes_is_zero);

LONE_LISP_PRIMITIVE(bytes_read_u8);
LONE_LISP_PRIMITIVE(bytes_read_s8);

LONE_LISP_PRIMITIVE(bytes_write_u8);
LONE_LISP_PRIMITIVE(bytes_write_s8);

LONE_LISP_PRIMITIVE(bytes_read_u16);
LONE_LISP_PRIMITIVE(bytes_read_s16);
LONE_LISP_PRIMITIVE(bytes_read_u32);
LONE_LISP_PRIMITIVE(bytes_read_s32);

LONE_LISP_PRIMITIVE(bytes_write_u16);
LONE_LISP_PRIMITIVE(bytes_write_s16);
LONE_LISP_PRIMITIVE(bytes_write_u32);
LONE_LISP_PRIMITIVE(bytes_write_s32);

LONE_LISP_PRIMITIVE(bytes_read_u16le);
LONE_LISP_PRIMITIVE(bytes_read_s16le);
LONE_LISP_PRIMITIVE(bytes_read_u32le);
LONE_LISP_PRIMITIVE(bytes_read_s32le);

LONE_LISP_PRIMITIVE(bytes_read_u16be);
LONE_LISP_PRIMITIVE(bytes_read_s16be);
LONE_LISP_PRIMITIVE(bytes_read_u32be);
LONE_LISP_PRIMITIVE(bytes_read_s32be);

LONE_LISP_PRIMITIVE(bytes_write_u16le);
LONE_LISP_PRIMITIVE(bytes_write_s16le);
LONE_LISP_PRIMITIVE(bytes_write_u32le);
LONE_LISP_PRIMITIVE(bytes_write_s32le);

LONE_LISP_PRIMITIVE(bytes_write_u16be);
LONE_LISP_PRIMITIVE(bytes_write_s16be);
LONE_LISP_PRIMITIVE(bytes_write_u32be);
LONE_LISP_PRIMITIVE(bytes_write_s32be);

#endif /* LONE_LISP_MODULES_INTRINSIC_BYTES_HEADER */
