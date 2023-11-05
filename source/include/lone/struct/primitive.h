#ifndef LONE_STRUCT_PRIMITIVE_SOURCE_HEADER
#define LONE_STRUCT_PRIMITIVE_SOURCE_HEADER

#include <lone/types.h>

typedef struct lone_value *(*lone_primitive)(struct lone_lisp *lone,
                                             struct lone_value *module,
                                             struct lone_value *environment,
                                             struct lone_value *arguments,
                                             struct lone_value *closure);

struct lone_primitive {
	struct lone_value *name;
	lone_primitive function;
	struct lone_value *closure;
	struct lone_function_flags flags;    /* primitives always accept variable arguments */
};

#endif /* LONE_STRUCT_PRIMITIVE_SOURCE_HEADER */
