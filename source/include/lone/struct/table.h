#ifndef LONE_STRUCT_TABLE_SOURCE_HEADER
#define LONE_STRUCT_TABLE_SOURCE_HEADER

#include <lone/types.h>

struct lone_table_entry {
	struct lone_value *key;
	struct lone_value *value;
};

struct lone_table {
	size_t count;
	size_t capacity;
	struct lone_table_entry *entries;
	struct lone_value *prototype;
};

#endif /* LONE_STRUCT_TABLE_SOURCE_HEADER */
