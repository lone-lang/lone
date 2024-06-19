/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_VALUE_SYMBOL_HEADER
#define LONE_LISP_VALUE_SYMBOL_HEADER

#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone symbols are like lone texts but are interned in a table.       │
   │    Symbol table interning deduplicates them in memory,                 │
   │    enabling fast identity-based comparisons via pointer equality.      │
   │    However, this means they won't be garbage collected.                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_intern(struct lone_lisp *lone,
		unsigned char *bytes, size_t count, bool should_deallocate);

struct lone_lisp_value lone_lisp_intern_bytes(struct lone_lisp *lone,
		struct lone_bytes bytes, bool should_deallocate);

struct lone_lisp_value lone_lisp_intern_c_string(struct lone_lisp *lone,
		char *c_string);

struct lone_lisp_value lone_lisp_intern_text(struct lone_lisp *lone,
		struct lone_lisp_value text);

#endif /* LONE_LISP_VALUE_SYMBOL_HEADER */
