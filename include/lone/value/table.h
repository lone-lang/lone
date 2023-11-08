#ifndef LONE_VALUE_TABLE_HEADER
#define LONE_VALUE_TABLE_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Lone tables are openly addressed, linearly probed hash tables.      │
   │    Currently, lone tables use the FNV-1a hashing algorithm.            │
   │    They also strive to maintain a load factor of at most 0.5:          │
   │    tables will be rehashed once they're above half capacity.           │
   │    They do not use tombstones to delete keys.                          │
   │                                                                        │
   │    Tables are able to inherit from another table: missing keys         │
   │    are also looked up in the parent table. This is currently used      │
   │    to implement nested environments but will also serve as a           │
   │    prototype-based object system as in Javascript and Self.            │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_value *lone_table_create(struct lone_lisp *lone, size_t capacity, struct lone_value *prototype);

#endif /* LONE_VALUE_TABLE_HEADER */
