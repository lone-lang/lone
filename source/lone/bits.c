/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/bits.h>
#include <limits.h>

#define BITS_PER_LONG (sizeof(unsigned long) * CHAR_BIT)

struct bit_position {
	unsigned char *byte;
	unsigned char bit;
};

static struct bit_position
bit_position(void *bits, lone_size index)
{
	unsigned char *bytes = bits;

	return (struct bit_position) {
		.byte = bytes + (index / CHAR_BIT),
		.bit  = (CHAR_BIT - 1) - (index % CHAR_BIT),
	};
}

static bool get_bit(struct bit_position position)
{
	return (*position.byte) & (1 << position.bit);
}

static bool set_bit(struct bit_position position, bool bit)
{
	if (bit) {
		*position.byte |=  (1 << position.bit);
	} else {
		*position.byte &= ~(1 << position.bit);
	}

	return bit;
}

bool lone_bits_get(void *bits, lone_size index)
{
	return get_bit(bit_position(bits, index));
}

bool lone_bits_set(void *bits, lone_size index, bool bit)
{
	return set_bit(bit_position(bits, index), bit);
}

void lone_bits_mark(void *bits, lone_size index)
{
	lone_bits_set(bits, index, true);
}

void lone_bits_clear(void *bits, lone_size index)
{
	lone_bits_set(bits, index, false);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    The bitmap is stored MSB first within each byte.                    │
   │    Loading bytes as an unsigned long reverses that order.              │
   │    A byte swap restores MSB first order in the register.               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static unsigned long load_word_msb_first(const unsigned long *pointer)
{
	unsigned long word = *pointer;

#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	#if __SIZEOF_LONG__ == 8
		word = __builtin_bswap64(word);
	#elif __SIZEOF_LONG__ == 4
		word = __builtin_bswap32(word);
	#else
		#error "Unsupported sizeof(long)"
	#endif
#endif

	return word;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Shift the byte to the most significant position of the int          │
   │    so that __builtin_clz counts from the byte's MSB directly           │
   │    without requiring a post-hoc correction for promotion width.        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static lone_size find_first_one_in_byte(unsigned char byte)
{
	unsigned int word = (unsigned int) byte;
	word <<= (sizeof(int) - 1) * CHAR_BIT;
	return __builtin_clz(word);
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Number of leading bytes before the next aligned boundary.           │
   │    Clamped to size so that small bitmaps don't overshoot.              │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static lone_size count_leading_unaligned_bytes(const void *pointer, lone_size size)
{
	lone_size misalignment, leading;

	misalignment = ((uintptr_t) pointer) % sizeof(unsigned long);

	if (misalignment == 0) {
		return 0;
	}

	leading = sizeof(unsigned long) - misalignment;

	if (leading > size) {
		leading = size;
	}

	return leading;
}

/* ╭────────────────────────────────────────────────────────────────────────╮
   │                                                                        │
   │    Three phase scan:                                                   │
   │                                                                        │
   │        1. Leading unaligned bytes scanned individually                 │
   │        2. Aligned unsigned long words scanned with CLZ                 │
   │        3. Trailing bytes scanned individually                          │
   │                                                                        │
   │    Handles arbitrary pointer alignment without undefined behavior.     │
   │    Still processes most of the data at word granularity.               │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

static struct lone_optional_size lone_bits_scan(const void * restrict bits, lone_size size, bool bit)
{
	lone_size leading, trailing, word_count, bit_index, i;
	const unsigned long *words;
	const unsigned char *bytes;
	unsigned long word, mask;
	unsigned char byte;

	bytes = bits;
	bit_index = 0;
	mask = bit? 0 : ~0UL;

	/* leading unaligned bytes */
	leading = count_leading_unaligned_bytes(bits, size);

	for (i = 0; i < leading; ++i) {
		byte = bytes[i];
		if (byte != ((unsigned char) mask)) {
			byte = bit? byte : ~byte;
			return LONE_OPTIONAL_PRESENT_VALUE(size, bit_index + find_first_one_in_byte(byte));
		}
		bit_index += CHAR_BIT;
	}

	/* aligned words */
	words = (const unsigned long *) (bytes + leading);
	word_count = (size - leading) / sizeof(*words);

	for (i = 0; i < word_count; ++i) {
		word = load_word_msb_first(&words[i]);
		if (word != mask) {
			word = bit? word : ~word;
			return LONE_OPTIONAL_PRESENT_VALUE(size, bit_index + __builtin_clzl(word));
		}
		bit_index += BITS_PER_LONG;
	}

	/* trailing bytes */
	bytes = (const unsigned char *) (words + word_count);
	trailing = (size - leading) % sizeof(unsigned long);

	for (i = 0; i < trailing; ++i) {
		byte = bytes[i];
		if (byte != ((unsigned char) mask)) {
			byte = bit? byte : ~byte;
			return LONE_OPTIONAL_PRESENT_VALUE(size, bit_index + find_first_one_in_byte(byte));
		}
		bit_index += CHAR_BIT;
	}

	return LONE_OPTIONAL_ABSENT_VALUE(size);
}

struct lone_optional_size lone_bits_find_first_one(const void * restrict bits, lone_size size)
{
	return lone_bits_scan(bits, size, 1);
}

struct lone_optional_size lone_bits_find_first_zero(const void * restrict bits, lone_size size)
{
	return lone_bits_scan(bits, size, 0);
}
