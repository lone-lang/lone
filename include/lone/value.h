#ifndef LONE_VALUE_HEADER
#define LONE_VALUE_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Constructor for a general lone value.                               │
   │    Completely uninitialized.                                           │
   │    Meant for other constructors to use.                                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
struct lone_value *lone_value_create(struct lone_lisp *lone);

#endif /* LONE_VALUE_HEADER */
