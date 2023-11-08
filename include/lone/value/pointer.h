#ifndef LONE_VALUE_POINTER_HEADER
#define LONE_VALUE_POINTER_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone pointers do not own the data they point to.                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value *lone_pointer_create(struct lone_lisp *lone, void *pointer, enum lone_pointer_type type);

#endif /* LONE_VALUE_POINTER_HEADER */
