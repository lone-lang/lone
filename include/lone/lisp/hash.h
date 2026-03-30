/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_LISP_HASH_HEADER
#define LONE_LISP_HASH_HEADER

#include <lone/lisp/types.h>

size_t lone_lisp_hash(struct lone_lisp *lone, struct lone_lisp_value value);

size_t lone_lisp_hash_as_symbol(struct lone_lisp *lone, struct lone_bytes name);
size_t lone_lisp_hash_as_text(struct lone_lisp *lone, struct lone_bytes bytes);
size_t lone_lisp_hash_as_bytes(struct lone_lisp *lone, struct lone_bytes bytes);

#endif /* LONE_LISP_HASH_HEADER */
