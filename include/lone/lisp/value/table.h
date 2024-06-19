/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_VALUE_TABLE_HEADER
#define LONE_VALUE_TABLE_HEADER

#include <lone/lisp/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Hash table functions.                                               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_lisp_value lone_lisp_table_create(struct lone_lisp *lone,
		size_t capacity, struct lone_lisp_value prototype);

struct lone_lisp_value lone_lisp_table_get(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_lisp_value key);

void lone_lisp_table_set(struct lone_lisp *lone, struct lone_lisp_value table,
		struct lone_lisp_value key, struct lone_lisp_value value);

void lone_lisp_table_delete(struct lone_lisp *lone,
		struct lone_lisp_value table, struct lone_lisp_value key);

size_t lone_lisp_table_count(struct lone_lisp_value table);
struct lone_lisp_value lone_lisp_table_key_at(struct lone_lisp_value table, lone_size i);
struct lone_lisp_value lone_lisp_table_value_at(struct lone_lisp_value table, lone_size i);

#define LONE_LISP_TABLE_FOR_EACH(entry, table, i)                               \
	for ((i) = 0, (entry) = &(table).as.heap_value->as.table.entries[0];    \
	     (i) < (table).as.heap_value->as.table.count;                       \
	     ++(i), (entry) = &(table).as.heap_value->as.table.entries[i])

#endif /* LONE_VALUE_TABLE_HEADER */
