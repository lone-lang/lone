#ifndef LONE_STRUCT_POINTER_SOURCE_HEADER
#define LONE_STRUCT_POINTER_SOURCE_HEADER

#include <lone/types.h>

enum lone_pointer_type {
	LONE_TO_UNKNOWN,

	LONE_TO_U8,  LONE_TO_I8,
	LONE_TO_U16, LONE_TO_I16,
	LONE_TO_U32, LONE_TO_I32,
	LONE_TO_U64, LONE_TO_I64,
};

struct lone_pointer {
	enum lone_pointer_type type;
	void *address;
};

#endif /* LONE_STRUCT_POINTER_SOURCE_HEADER */
