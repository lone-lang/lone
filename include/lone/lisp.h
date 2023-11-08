#ifndef LONE_LISP_HEADER
#define LONE_LISP_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The lone lisp structure represents the lone lisp interpreter.       │
   │    A pointer to this structure is passed to nearly every function.     │
   │    It must be initialized before everything else since the memory      │
   │    allocation system is not functional without it.                     │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_lisp_initialize(struct lone_lisp *lone, struct lone_bytes memory, size_t heap_size, void *stack, struct lone_bytes random);

#endif /* LONE_LISP_HEADER */
