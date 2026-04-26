/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_HASH_SIPHASH_HEADER
#define LONE_HASH_SIPHASH_HEADER

#include <lone/types.h>

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    SipHash-2-4: a secure, fast non-cryptographic hash function         │
   │    designed for hash table keying. Provably a PRF under                │
   │    standard assumptions, preventing HashDoS attacks.                   │
   │                                                                        │
   │    Aumasson & Bernstein, 2012.                                         │
   │    https://cr.yp.to/siphash/siphash-20120918.pdf                       │
   │    https://github.com/veorq/SipHash                                    │
   │                                                                        │
   │    Core operation is ARX (add/rotate/xor) on four 64-bit words.        │
   │    Requires a 128-bit key for keyed hashing.                           │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

struct lone_hash_siphash_state {
	lone_u64 v0, v1, v2, v3;
	unsigned char buffer[8];
	size_t buffered;
	size_t total;
};

void lone_hash_siphash_initialize(struct lone_hash_siphash_state *state,
		lone_u64 k0, lone_u64 k1);
void lone_hash_siphash_update(struct lone_hash_siphash_state *state,
		struct lone_bytes data);
lone_u64 lone_hash_siphash_finish(struct lone_hash_siphash_state *state);

lone_u64
lone_hash_siphash(struct lone_bytes data, lone_u64 k0, lone_u64 k1);

#endif /* LONE_HASH_SIPHASH_HEADER */
