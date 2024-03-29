/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_UTILITIES_HEADER
#define LONE_UTILITIES_HEADER

#include <lone/types.h>

struct lone_value lone_apply_predicate(struct lone_lisp *lone, struct lone_value arguments, lone_predicate function);
struct lone_value lone_apply_comparator(struct lone_lisp *lone, struct lone_value arguments, lone_comparator function);
struct lone_bytes lone_join(struct lone_lisp *lone, struct lone_value separator, struct lone_value arguments, lone_predicate is_valid);
struct lone_bytes lone_concatenate(struct lone_lisp *lone, struct lone_value arguments, lone_predicate is_valid);

long lone_min(long x, long y);
long lone_max(long x, long y);

#endif /* LONE_UTILITIES_HEADER */

