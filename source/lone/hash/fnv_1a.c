/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/hash/fnv_1a.h>

unsigned long __attribute__((pure)) lone_hash_fnv_1a(struct lone_bytes data, unsigned long offset_basis)
{
	unsigned long hash = offset_basis;
	unsigned char *bytes = data.pointer;
	size_t count = data.count;

	while (count--) {
		hash ^= *bytes++;
		hash *= FNV_PRIME;
	}

	return hash;
}

void lone_hash_fnv_1a_initialize(struct lone_lisp *lone, struct lone_bytes random)
{
	lone->hash.fnv_1a.offset_basis = lone_hash_fnv_1a(random, FNV_OFFSET_BASIS);
}
