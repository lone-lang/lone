/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_HASH_HEADER
#define LONE_HASH_HEADER

#include <lone/types.h>

void lone_hash_initialize(struct lone_lisp *lone, struct lone_bytes random);
size_t lone_hash(struct lone_lisp *lone, struct lone_value value);

#endif /* LONE_HASH_HEADER */
