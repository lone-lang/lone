#ifndef LONE_MODULES_TEXT_HEADER
#define LONE_MODULES_TEXT_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Text operations.                                                    │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_module_text_initialize(struct lone_lisp *lone);

LONE_PRIMITIVE(text_join);
LONE_PRIMITIVE(text_concatenate);

#endif /* LONE_MODULES_TEXT_HEADER */
