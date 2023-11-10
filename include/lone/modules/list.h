#ifndef LONE_MODULES_LIST_HEADER
#define LONE_MODULES_LIST_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    List operations.                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_module_list_initialize(struct lone_lisp *lone);

LONE_PRIMITIVE(list_construct);
LONE_PRIMITIVE(list_first);
LONE_PRIMITIVE(list_rest);
LONE_PRIMITIVE(list_map);
LONE_PRIMITIVE(list_reduce);
LONE_PRIMITIVE(list_flatten);

#endif /* LONE_MODULES_LIST_HEADER */
