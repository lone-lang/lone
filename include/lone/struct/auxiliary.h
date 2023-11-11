#ifndef LONE_STRUCT_AUXILIARY_HEADER
#define LONE_STRUCT_AUXILIARY_HEADER

#include <linux/auxvec.h>

#include <lone/types.h>

struct auxiliary {
	long type;
	union {
		char *c_string;
		void *pointer;
		long integer;
	} as;
};

#endif /* LONE_STRUCT_AUXILIARY_HEADER */
