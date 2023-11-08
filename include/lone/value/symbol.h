#ifndef LONE_VALUE_SYMBOL_HEADER
#define LONE_VALUE_SYMBOL_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone symbols are like lone texts but are interned in a table.       │
   │    Symbol table interning deduplicates them in memory,                 │
   │    enabling fast identity-based comparisons via pointer equality.      │
   │    However, this means they won't be garbage collected.                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value *lone_symbol_transfer(struct lone_lisp *lone, unsigned char *text, size_t length, bool should_deallocate);
struct lone_value *lone_symbol_transfer_bytes(struct lone_lisp *lone, struct lone_bytes bytes, bool should_deallocate);
struct lone_value *lone_symbol_create(struct lone_lisp *lone, unsigned char *text, size_t length);

#endif /* LONE_VALUE_SYMBOL_HEADER */
