/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_VALUE_INTEGER_HEADER
#define LONE_VALUE_INTEGER_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone integers are currently signed fixed-length integers.           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value lone_integer_create(long integer);
struct lone_value lone_integer_parse(struct lone_lisp *lone, unsigned char *digits, size_t count);

struct lone_value lone_zero(void);
struct lone_value lone_one(void);
struct lone_value lone_minus_one(void);

#endif /* LONE_VALUE_INTEGER_HEADER */
