/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/hash/fnv_1a.h>
#include <lone/types.h>

void lone_hash_fnv_1a_initialize(struct lone_hash_fnv_1a_state *state, unsigned long offset_basis)
{
	state->hash = offset_basis;
}

void lone_hash_fnv_1a_update(struct lone_hash_fnv_1a_state *state, struct lone_bytes data)
{
	unsigned char *bytes = data.pointer;
	size_t count = data.count;

	while (count--) {
		state->hash ^= *bytes++;
		state->hash *= FNV_PRIME;
	}
}

unsigned long lone_hash_fnv_1a_finish(struct lone_hash_fnv_1a_state *state)
{
	return state->hash;
}

unsigned long __attribute__((pure)) lone_hash_fnv_1a(struct lone_bytes data, unsigned long offset_basis)
{
	struct lone_hash_fnv_1a_state state;

	lone_hash_fnv_1a_initialize(&state, offset_basis);
	lone_hash_fnv_1a_update(&state, data);

	return lone_hash_fnv_1a_finish(&state);
}
