/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_HASH_HEADER
#define LONE_LISP_HASH_HEADER

#include <lone/lisp/types.h>

lone_hash lone_lisp_hash_of(struct lone_lisp *lone, struct lone_lisp_value value);

lone_hash lone_lisp_hash_as_symbol(struct lone_lisp *lone, struct lone_bytes name);
lone_hash lone_lisp_hash_as_text(struct lone_lisp *lone, struct lone_bytes bytes);
lone_hash lone_lisp_hash_as_bytes(struct lone_lisp *lone, struct lone_bytes bytes);

lone_hash lone_lisp_value_compute_and_store_hash(struct lone_lisp *lone,
		struct lone_lisp_value value);

#endif /* LONE_LISP_HASH_HEADER */
