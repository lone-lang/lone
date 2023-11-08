#ifndef LONE_HASH_FNV_1A_HEADER
#define LONE_HASH_FNV_1A_HEADER

#include <asm/bitsperlong.h>

#include <lone/types.h>
#include <lone/struct/bytes.h>

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

void lone_hash_fnv_1a_initialize(struct lone_lisp *lone, struct lone_bytes random);

unsigned long
__attribute__((pure))
lone_hash_fnv_1a(struct lone_bytes data, unsigned long offset_basis);

#endif /* LONE_HASH_FNV_1A_HEADER */
