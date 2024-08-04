/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_SEGMENT_HEADER
#define LONE_LISP_SEGMENT_HEADER

#include <lone/types.h>
#include <lone/segment.h>

#include <lone/lisp/types.h>

struct lone_lisp_value lone_lisp_segment_read_descriptor(struct lone_lisp *lone, lone_elf_native_segment *segment);

#endif /* LONE_LISP_SEGMENT_HEADER */
