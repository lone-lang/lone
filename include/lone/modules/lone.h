#ifndef LONE_MODULES_LONE_HEADER
#define LONE_MODULES_LONE_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

/* ╭───────────────────┨ LONE LISP PRIMITIVE FUNCTIONS ┠────────────────────╮
   │                                                                        │
   │    Lone lisp functions implemented in C.                               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

void lone_module_lone_initialize(struct lone_lisp *lone);

LONE_PRIMITIVE(lone_begin);
LONE_PRIMITIVE(lone_when);
LONE_PRIMITIVE(lone_unless);
LONE_PRIMITIVE(lone_if);
LONE_PRIMITIVE(lone_let);
LONE_PRIMITIVE(lone_set);
LONE_PRIMITIVE(lone_quote);
LONE_PRIMITIVE(lone_quasiquote);
LONE_PRIMITIVE(lone_lambda);
LONE_PRIMITIVE(lone_lambda_bang);
LONE_PRIMITIVE(lone_lambda_star);
LONE_PRIMITIVE(lone_is_list);
LONE_PRIMITIVE(lone_is_vector);
LONE_PRIMITIVE(lone_is_table);
LONE_PRIMITIVE(lone_is_symbol);
LONE_PRIMITIVE(lone_is_text);
LONE_PRIMITIVE(lone_is_integer);
LONE_PRIMITIVE(lone_is_identical);
LONE_PRIMITIVE(lone_is_equivalent);
LONE_PRIMITIVE(lone_is_equal);
LONE_PRIMITIVE(lone_print);

#endif /* LONE_MODULES_LONE_HEADER */
