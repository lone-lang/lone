#ifndef LONE_VALUE_LIST_HEADER
#define LONE_VALUE_LIST_HEADER

#include <stdarg.h>

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

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    List manipulation functions.                                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value *lone_list_first(struct lone_value *value);
struct lone_value *lone_list_rest(struct lone_value *value);
struct lone_value *lone_list_set_first(struct lone_value *list, struct lone_value *value);
struct lone_value *lone_list_set_rest(struct lone_value *list, struct lone_value *rest);
struct lone_value *lone_list_append(struct lone_lisp *lone, struct lone_value *list, struct lone_value *value);
struct lone_value *lone_list_build(struct lone_lisp *lone, size_t count, ...);
struct lone_value *lone_list_to_vector(struct lone_lisp *lone, struct lone_value *list);
struct lone_value *lone_list_flatten(struct lone_lisp *lone, struct lone_value *list);
struct lone_bytes lone_join(struct lone_lisp *lone, struct lone_value *separator, struct lone_value *arguments, lone_predicate is_valid);
struct lone_bytes lone_concatenate(struct lone_lisp *lone, struct lone_value *arguments, lone_predicate is_valid);

#endif /* LONE_VALUE_LIST_HEADER */
