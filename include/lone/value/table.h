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

struct lone_value lone_table_get(struct lone_lisp *lone, struct lone_value table, struct lone_value key);
void lone_table_set(struct lone_lisp *lone, struct lone_value table, struct lone_value key, struct lone_value value);
void lone_table_delete(struct lone_lisp *lone, struct lone_value table, struct lone_value key);

size_t lone_table_count(struct lone_value table);
struct lone_value lone_table_key_at(struct lone_value table, lone_size i);
struct lone_value lone_table_value_at(struct lone_value table, lone_size i);

#define LONE_TABLE_FOR_EACH(key, value, table, i)                               \
	for ((i) = 0,                                                           \
	     (key) = lone_table_key_at((table), 0),                             \
	     (value) = lone_table_value_at((table), 0);                         \
	     (i) < lone_table_count((table));                                   \
	     ++(i),                                                             \
	     (key) = lone_table_key_at((table), i),                             \
	     (value) = lone_table_value_at((table), i))

#endif /* LONE_VALUE_TABLE_HEADER */
