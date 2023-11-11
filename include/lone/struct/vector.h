/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_STRUCT_VECTOR_HEADER
#define LONE_STRUCT_VECTOR_HEADER

#include <lone/types.h>

struct lone_vector {
	struct lone_value **values;
	size_t count;
	size_t capacity;
};

#endif /* LONE_STRUCT_VECTOR_HEADER */
