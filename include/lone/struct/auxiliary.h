/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_STRUCT_AUXILIARY_HEADER
#define LONE_STRUCT_AUXILIARY_HEADER

#include <linux/auxvec.h>

#include <lone/types.h>

struct auxiliary_value {
	union {
		char *c_string;
		void *pointer;
		long integer;
		unsigned long unsigned_integer;
	} as;
};

struct auxiliary_vector {
	long type;
	struct auxiliary_value value;
};

#endif /* LONE_STRUCT_AUXILIARY_HEADER */
