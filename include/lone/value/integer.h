/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_VALUE_INTEGER_HEADER
#define LONE_VALUE_INTEGER_HEADER

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone integers are currently signed fixed-length integers.           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value *lone_integer_create(struct lone_lisp *lone, long integer);
struct lone_value *lone_integer_parse(struct lone_lisp *lone, unsigned char *digits, size_t count);

#endif /* LONE_VALUE_INTEGER_HEADER */
