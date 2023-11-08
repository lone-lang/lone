#ifndef LONE_VALUE_LIST_HEADER
#define LONE_VALUE_LIST_HEADER

#include <lone/types.h>

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

struct lone_value *lone_list_create(struct lone_lisp *lone, struct lone_value *first, struct lone_value *rest);
struct lone_value *lone_list_create_nil(struct lone_lisp *lone);
struct lone_value *lone_nil(struct lone_lisp *lone);

#endif /* LONE_VALUE_LIST_HEADER */
