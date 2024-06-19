/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_UTILITIES_HEADER
#define LONE_LISP_UTILITIES_HEADER

#include <lone/lisp/types.h>

struct lone_lisp_value lone_lisp_apply_predicate(struct lone_lisp *lone,
		struct lone_lisp_value arguments, lone_lisp_predicate_function function);

struct lone_lisp_value lone_lisp_apply_comparator(struct lone_lisp *lone,
		struct lone_lisp_value arguments, lone_lisp_comparator_function function);

struct lone_bytes lone_lisp_join(struct lone_lisp *lone,
		struct lone_lisp_value separator, struct lone_lisp_value arguments,
		lone_lisp_predicate_function is_valid);

struct lone_bytes lone_lisp_concatenate(struct lone_lisp *lone,
		struct lone_lisp_value arguments, lone_lisp_predicate_function is_valid);

#endif /* LONE_LISP_UTILITIES_HEADER */
