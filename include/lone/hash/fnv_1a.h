/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_HASH_FNV_1A_HEADER
#define LONE_HASH_FNV_1A_HEADER

#include <lone/definitions.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    https://en.wikipedia.org/wiki/FNV_hash                              │
   │    https://datatracker.ietf.org/doc/draft-eastlake-fnv/                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#if __BITS_PER_LONG == 64
	#define FNV_PRIME 0x00000100000001B3UL
	#define FNV_OFFSET_BASIS 0xCBF29CE484222325UL
#elif __BITS_PER_LONG == 32
	#define FNV_PRIME 0x01000193UL
	#define FNV_OFFSET_BASIS 0x811C9DC5
#else
	#error "Unsupported architecture"
#endif

struct lone_bytes;

struct lone_hash_fnv_1a_state {
	unsigned long hash;
};

void lone_hash_fnv_1a_initialize(struct lone_hash_fnv_1a_state *state, unsigned long offset_basis);
void lone_hash_fnv_1a_update(struct lone_hash_fnv_1a_state *state, struct lone_bytes data);
unsigned long lone_hash_fnv_1a_finish(struct lone_hash_fnv_1a_state *state);

unsigned long
__attribute__((pure))
lone_hash_fnv_1a(struct lone_bytes data, unsigned long offset_basis);

#endif /* LONE_HASH_FNV_1A_HEADER */
