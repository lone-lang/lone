#ifndef LONE_STRUCT_PRIMITIVE_HEADER
#define LONE_STRUCT_PRIMITIVE_HEADER

#include <lone/types.h>

struct lone_primitive {
	struct lone_value *name;
	lone_primitive function;
	struct lone_value *closure;
	struct lone_function_flags flags;    /* primitives always accept variable arguments */
};

#endif /* LONE_STRUCT_PRIMITIVE_HEADER */
