/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_UNICODE_HEADER
#define LONE_UNICODE_HEADER

#include <lone/types.h>

/* ╭──────────────────────────────┨ UNICODE ┠───────────────────────────────╮
   │                                                                        │
   │    UTF-8 decoder, encoder and validator.                               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

#define LONE_UNICODE_CODE_POINT_MAX          0x10FFFF
#define LONE_UNICODE_SURROGATE_MIN           0xD800
#define LONE_UNICODE_SURROGATE_MAX           0xDFFF

#endif /* LONE_UNICODE_HEADER */
