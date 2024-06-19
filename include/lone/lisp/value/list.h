/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_VALUE_LIST_HEADER
#define LONE_LISP_VALUE_LIST_HEADER

#include <stdarg.h>

#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone lists are pairs of lone values: first and rest.                │
   │    Usually first is the element while rest is another pair.            │
   │    This explains their names. Also called car and cdr.                 │
   │                                                                        │
   │    Although often the case, rest need not be another pair.             │
   │    Any other object may be set: (1 . 2); first = 1, rest = 2.          │
   │    So rest could also be named second.                                 │
   │                                                                        │
   │    A list with null first and rest pointers is known as nil.           │
   │    It is provided as a constant by the lone interpreter.               │
   │    Their presence in the rest of a list marks its end.                 │
   │    New nil values may be created by C code that builds lists.          │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_list_create(struct lone_lisp *lone,
		struct lone_lisp_value first, struct lone_lisp_value rest);

struct lone_lisp_value lone_lisp_list_create_nil(struct lone_lisp *lone);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    List manipulation functions.                                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

bool lone_lisp_list_has_rest(struct lone_lisp_value value);
struct lone_lisp_value lone_lisp_list_first(struct lone_lisp_value value);
struct lone_lisp_value lone_lisp_list_rest(struct lone_lisp_value value);

struct lone_lisp_value lone_lisp_list_set_first(struct lone_lisp *lone,
		struct lone_lisp_value list, struct lone_lisp_value value);

struct lone_lisp_value lone_lisp_list_set_rest(struct lone_lisp *lone,
		struct lone_lisp_value list, struct lone_lisp_value rest);

struct lone_lisp_value lone_lisp_list_append(struct lone_lisp *lone,
		struct lone_lisp_value *first, struct lone_lisp_value *head,
		struct lone_lisp_value value);

bool lone_lisp_list_destructure(struct lone_lisp_value list, size_t count, ...);
struct lone_lisp_value lone_lisp_list_build(struct lone_lisp *lone, size_t count, ...);
struct lone_lisp_value lone_lisp_list_flatten(struct lone_lisp *lone, struct lone_lisp_value list);
struct lone_lisp_value lone_lisp_list_to_vector(struct lone_lisp *lone, struct lone_lisp_value list);

#endif /* LONE_LISP_VALUE_LIST_HEADER */
