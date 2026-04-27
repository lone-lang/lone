/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/hash/siphash.h>
#include <lone/types.h>

static lone_u64 lone_hash_siphash_rotate_left(lone_u64 x, int b)
{
	return (x << b) | (b? (x >> (64 - b)) : 0);
}

#define SIPROUND do {                                                                              \
    state->v0 += state->v1;                                                                        \
    state->v1 = lone_hash_siphash_rotate_left(state->v1, 13);                                      \
    state->v1 ^= state->v0;                                                                        \
    state->v0 = lone_hash_siphash_rotate_left(state->v0, 32);                                      \
    state->v2 += state->v3;                                                                        \
    state->v3 = lone_hash_siphash_rotate_left(state->v3, 16);                                      \
    state->v3 ^= state->v2;                                                                        \
    state->v0 += state->v3;                                                                        \
    state->v3 = lone_hash_siphash_rotate_left(state->v3, 21);                                      \
    state->v3 ^= state->v0;                                                                        \
    state->v2 += state->v1;                                                                        \
    state->v1 = lone_hash_siphash_rotate_left(state->v1, 17);                                      \
    state->v1 ^= state->v2;                                                                        \
    state->v2 = lone_hash_siphash_rotate_left(state->v2, 32);                                      \
} while (0)

static void lone_hash_siphash_compress(struct lone_hash_siphash_state *state, lone_u64 m)
{
	state->v3 ^= m;
	SIPROUND;
	SIPROUND;
	state->v0 ^= m;
}

void lone_hash_siphash_initialize(struct lone_hash_siphash_state *state,
		lone_u64 k0, lone_u64 k1)
{
	state->v0       = k0 ^ 0x736f6d6570736575UL;
	state->v1       = k1 ^ 0x646f72616e646f6dUL;
	state->v2       = k0 ^ 0x6c7967656e657261UL;
	state->v3       = k1 ^ 0x7465646279746573UL;
	state->buffered = 0;
	state->total    = 0;
}

void lone_hash_siphash_update(struct lone_hash_siphash_state *state, struct lone_bytes data)
{
	const unsigned char *p = data.pointer;
	size_t count = data.count;
	size_t i;

	state->total += count;

	if (state->buffered > 0) {
		while (state->buffered < 8 && count > 0) {
			state->buffer[state->buffered++] = *p++;
			--count;
		}

		if (state->buffered < 8) {
			return;
		}

		lone_hash_siphash_compress(state, lone_u64le_read(state->buffer));
		state->buffered = 0;
	}

	while (count >= 8) {
		lone_hash_siphash_compress(state, lone_u64le_read(p));
		p += 8;
		count -= 8;
	}

	for (i = 0; i < count; ++i) {
		state->buffer[i] = p[i];
	}

	state->buffered = count;
}

lone_u64 lone_hash_siphash_finish(struct lone_hash_siphash_state *state)
{
	lone_u64 b;
	size_t i;

	b = ((lone_u64) state->total & 0xff) << 56;

	for (i = 0; i < state->buffered; ++i) {
		b |= (lone_u64) state->buffer[i] << (i * 8);
	}

	lone_hash_siphash_compress(state, b);

	state->v2 ^= 0xff;
	SIPROUND;
	SIPROUND;
	SIPROUND;
	SIPROUND;

	return state->v0 ^ state->v1 ^ state->v2 ^ state->v3;
}

#undef SIPROUND

lone_u64 lone_hash_siphash(struct lone_bytes data, lone_u64 k0, lone_u64 k1)
{
	struct lone_hash_siphash_state state;

	lone_hash_siphash_initialize(&state, k0, k1);
	lone_hash_siphash_update(&state, data);

	return lone_hash_siphash_finish(&state);
}
