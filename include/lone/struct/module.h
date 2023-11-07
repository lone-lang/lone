#ifndef LONE_STRUCT_MODULE_HEADER
#define LONE_STRUCT_MODULE_HEADER

#include <lone/types.h>

struct lone_module {
	struct lone_value *name;
	struct lone_value *environment;
	struct lone_value *exports;
};

#endif /* LONE_STRUCT_MODULE_HEADER */
