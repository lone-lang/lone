#ifndef LONE_VALUE_VECTOR_HEADER
#define LONE_VALUE_VECTOR_HEADER

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

struct lone_value *lone_vector_create(struct lone_lisp *lone, size_t capacity);

#endif /* LONE_VALUE_VECTOR_HEADER */
