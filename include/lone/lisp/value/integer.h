/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_VALUE_INTEGER_HEADER
#define LONE_LISP_VALUE_INTEGER_HEADER

#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone integers are currently signed fixed-length integers.           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_integer_create(lone_lisp_integer integer);
struct lone_lisp_value lone_lisp_integer_from_pointer(void *pointer);

struct lone_lisp_value lone_lisp_integer_parse(struct lone_lisp *lone,
		unsigned char *digits, size_t count);

struct lone_lisp_value lone_lisp_zero(void);
struct lone_lisp_value lone_lisp_one(void);
struct lone_lisp_value lone_lisp_minus_one(void);

#endif /* LONE_LISP_VALUE_INTEGER_HEADER */
