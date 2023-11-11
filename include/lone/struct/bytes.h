/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_STRUCT_BYTES_HEADER
#define LONE_STRUCT_BYTES_HEADER

#include <lone/types.h>

struct lone_bytes {
	size_t count;
	unsigned char *pointer;
};

#endif /* LONE_STRUCT_BYTES_HEADER */
