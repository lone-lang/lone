/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_RANDOM_HEADER
#define LONE_RANDOM_HEADER

#include <lone/types.h>

void lone_random_with_urandom_fallback(
		struct lone_auxiliary_vector *auxiliary_vector,
		struct lone_bytes buffer);

#endif /* LONE_RANDOM_HEADER */
