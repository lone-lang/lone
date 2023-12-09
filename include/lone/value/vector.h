/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_VALUE_VECTOR_HEADER
#define LONE_VALUE_VECTOR_HEADER

#include <stdarg.h>

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone vectors are simple dynamic arrays of lone values.              │
   │    They grow automatically as elements are added.                      │
   │    Any index may be used regardless of current length:                 │
   │    all the elements remain unset as the array grows.                   │
   │    Unset elements are null pointers which are currently                │
   │    converted to nil automatically but might one day serve              │
   │    as an undefined value like in other languages.                      │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value lone_vector_create(struct lone_lisp *lone, size_t capacity);

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Functions for vectors, lone's dynamic arrays.                       │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_vector_resize(struct lone_lisp *lone, struct lone_value *vector, size_t new_capacity);
struct lone_value *lone_vector_get_value_at(struct lone_lisp *lone, struct lone_value *vector, size_t i);
struct lone_value *lone_vector_get(struct lone_lisp *lone, struct lone_value *vector, struct lone_value *index);
void lone_vector_set_value_at(struct lone_lisp *lone, struct lone_value *vector, size_t i, struct lone_value *value);
void lone_vector_set(struct lone_lisp *lone, struct lone_value *vector, struct lone_value *index, struct lone_value *value);
void lone_vector_push(struct lone_lisp *lone, struct lone_value *vector, struct lone_value *value);
void lone_vector_push_va_list(struct lone_lisp *lone, struct lone_value *vector, size_t count, va_list arguments);
void lone_vector_push_all(struct lone_lisp *lone, struct lone_value *vector, size_t count, ...);
struct lone_value *lone_vector_build(struct lone_lisp *lone, size_t count, ...);
bool lone_vector_contains(struct lone_value *vector, struct lone_value *value);

#define LONE_VECTOR_FOR_EACH(entry, vector, i)                         \
	for ((i) = 0, (entry) = (vector)->vector.values[0];            \
	     (i) < (vector)->vector.count;                             \
	     ++(i), (entry) = (vector)->vector.values[i])

#endif /* LONE_VALUE_VECTOR_HEADER */
