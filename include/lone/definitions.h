/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │                                      bits = 32    |    bits = 64       │
   │    digits = ceil(bits * log10(2)) =  10           |    20              │
   │                                                                        │
   │    https://en.wikipedia.org/wiki/FNV_hash                              │
   │    https://datatracker.ietf.org/doc/draft-eastlake-fnv/                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#if __BITS_PER_LONG == 64
	#define DECIMAL_DIGITS_PER_LONG 20
	#define FNV_PRIME 0x00000100000001B3UL
	#define FNV_OFFSET_BASIS 0xCBF29CE484222325UL
#elif __BITS_PER_LONG == 32
	#define DECIMAL_DIGITS_PER_LONG 10
	#define FNV_PRIME 0x01000193UL
	#define FNV_OFFSET_BASIS 0x811C9DC5
#else
	#error "Unsupported architecture"
#endif

#ifndef LONE_BUFFER_SIZE
	#define LONE_BUFFER_SIZE 4096
#endif

#ifndef LONE_MEMORY_SIZE
	#define LONE_MEMORY_SIZE (1024 * 1024)
#endif

#ifndef LONE_ALIGNMENT
	#define LONE_ALIGNMENT 16
#endif
