/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_VALUE_TABLE_HEADER
#define LONE_VALUE_TABLE_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Hash table functions.                                               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value lone_table_create(struct lone_lisp *lone, size_t capacity, struct lone_value prototype);

struct lone_value *lone_table_get(struct lone_lisp *lone, struct lone_value *table, struct lone_value *key);
void lone_table_set(struct lone_lisp *lone, struct lone_value *table, struct lone_value *key, struct lone_value *value);
void lone_table_delete(struct lone_lisp *lone, struct lone_value *table, struct lone_value *key);

#define LONE_TABLE_FOR_EACH(entry, table, i)                                    \
	for ((i) = 0, (entry) = &(table).as.heap_value->as.table.entries[0];    \
	     (i) < (table).as.heap_value->as.table.count;                       \
	     ++(i), (entry) = &(table).as.heap_value->as.table.entries[i])

#endif /* LONE_VALUE_TABLE_HEADER */
