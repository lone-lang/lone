/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_HASH_FNV_1A_HEADER
#define LONE_HASH_FNV_1A_HEADER

#include <lone/definitions.h>
#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    https://en.wikipedia.org/wiki/FNV_hash                              │
   │    https://datatracker.ietf.org/doc/draft-eastlake-fnv/                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#define FNV_PRIME_32        0x01000193UL
#define FNV_OFFSET_BASIS_32 0x811C9DC5UL

#define FNV_PRIME_64        0x00000100000001B3UL
#define FNV_OFFSET_BASIS_64 0xCBF29CE484222325UL

#define FNV_PRIME           FNV_PRIME_64
#define FNV_OFFSET_BASIS    FNV_OFFSET_BASIS_64

struct lone_bytes;

struct lone_hash_fnv_1a_state {
	lone_hash hash;
};

void lone_hash_fnv_1a_initialize(struct lone_hash_fnv_1a_state *state, lone_hash offset_basis);
void lone_hash_fnv_1a_update(struct lone_hash_fnv_1a_state *state, struct lone_bytes data);
lone_hash lone_hash_fnv_1a_finish(struct lone_hash_fnv_1a_state *state);

lone_hash
__attribute__((pure))
lone_hash_fnv_1a(struct lone_bytes data, lone_hash offset_basis);

#endif /* LONE_HASH_FNV_1A_HEADER */
