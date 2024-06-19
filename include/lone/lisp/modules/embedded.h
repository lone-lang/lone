/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_MODULES_EMBEDDED_HEADER
#define LONE_LISP_MODULES_EMBEDDED_HEADER

#include <lone/types.h>
#include <lone/lisp/types.h>

void lone_lisp_modules_embedded_load(struct lone_lisp *lone, lone_elf_segment *values);

#endif /* LONE_LISP_MODULES_EMBEDDED_HEADER */
