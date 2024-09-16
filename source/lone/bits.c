#include <lone/bits.h>

struct bit_position {
	unsigned char *byte;
	unsigned char bit;
};

static struct bit_position
bit_position(void *bits, lone_size index)
{
	unsigned char *bytes = bits;

	return (struct bit_position) {
		.byte = bytes + (index / 8),
		.bit  = 8 - (index % 8) - 1,
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

lone_size lone_bits_find_first_one(const void * restrict bits, lone_size size)
{
	lone_size bit_offset, bit_position, i;
	const unsigned char * restrict bytes;
	unsigned char byte;

	for (bytes = bits, i = 0; i < size; ++i) {
		byte = bytes[i];

		if (byte != 0) {
			bit_offset = i * 8 * sizeof(byte);
			bit_position = __builtin_clz(byte);

			/* integer promotion adds leading bits */
			bit_position -= ((sizeof(int) - 1) * 8);

			return bit_offset + bit_position;
		} else {
			continue;
		}
	}

	return -1;
}

lone_size lone_bits_find_first_zero(const void * restrict bits, lone_size size)
{
	lone_size bit_offset, bit_position, i;
	const unsigned char * restrict bytes;
	unsigned char byte;

	for (bytes = bits, i = 0; i < size; ++i) {
		byte = bytes[i];

		if (byte != 255) {
			byte = ~byte;

			bit_offset = i * 8 * sizeof(byte);
			bit_position = __builtin_clz(byte);

			/* integer promotion adds leading bits */
			bit_position -= ((sizeof(int) - 1) * 8);

			return bit_offset + bit_position;
		} else {
			continue;
		}
	}

	return -1;
}
