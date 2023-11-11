/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_STRUCT_POINTER_HEADER
#define LONE_STRUCT_POINTER_HEADER

#include <lone/types.h>

struct lone_pointer {
	enum lone_pointer_type type;
	void *address;
};

#endif /* LONE_STRUCT_POINTER_HEADER */
