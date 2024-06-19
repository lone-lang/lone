/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_VALUE_VECTOR_HEADER
#define LONE_LISP_VALUE_VECTOR_HEADER

#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone vectors are simple dynamic arrays of lone values.              │
   │    They grow automatically as elements are added.                      │
   │    Any index may be used regardless of current length:                 │
   │    all the elements remain unset as the array grows.                   │
   │    The array is zero filled which makes unset elements nil.            │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_vector_create(struct lone_lisp *lone, size_t capacity);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Functions for vectors, lone's dynamic arrays.                       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

size_t lone_lisp_vector_count(struct lone_lisp_value vector);

void lone_lisp_vector_resize(struct lone_lisp *lone,
		struct lone_lisp_value vector, size_t new_capacity);

struct lone_lisp_value lone_lisp_vector_get_value_at(struct lone_lisp_value vector, size_t i);

struct lone_lisp_value lone_lisp_vector_get(struct lone_lisp *lone,
		struct lone_lisp_value vector, struct lone_lisp_value index);

void lone_lisp_vector_set_value_at(struct lone_lisp *lone,
		struct lone_lisp_value vector, size_t i,
		struct lone_lisp_value value);

void lone_lisp_vector_set(struct lone_lisp *lone, struct lone_lisp_value vector,
		struct lone_lisp_value index, struct lone_lisp_value value);

void lone_lisp_vector_push(struct lone_lisp *lone,
		struct lone_lisp_value vector, struct lone_lisp_value value);

void lone_lisp_vector_push_va_list(struct lone_lisp *lone,
		struct lone_lisp_value vector, size_t count, va_list arguments);

void lone_lisp_vector_push_all(struct lone_lisp *lone,
		struct lone_lisp_value vector, size_t count, ...);

struct lone_lisp_value lone_lisp_vector_build(struct lone_lisp *lone,
		size_t count, ...);

bool lone_lisp_vector_contains(struct lone_lisp_value vector,
		struct lone_lisp_value value);

#define LONE_LISP_VECTOR_FOR_EACH(entry, vector, i)                             \
	for ((i) = 0, (entry) = lone_lisp_vector_get_value_at((vector), 0);     \
	     (i) < lone_lisp_vector_count((vector));                            \
	     ++(i), (entry) = lone_lisp_vector_get_value_at((vector), i))

#endif /* LONE_LISP_VALUE_VECTOR_HEADER */
