/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_PRINTER_HEADER
#define LONE_LISP_PRINTER_HEADER

#include <lone/lisp/types.h>

/* ╭─────────────────────────┨ LONE LISP PRINTER ┠──────────────────────────╮
   │                                                                        │
   │    Transforms lone lisp objects into text and writes them out.         │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_lisp_print(struct lone_lisp *lone, struct lone_lisp_value value, int file_descriptor);

#endif /* LONE_LISP_PRINTER_HEADER */
