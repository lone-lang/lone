/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/memory/functions.h>
#include <lone/types.h>

int lone_memory_compare(const void *a, const void *b, size_t count)
{
	size_t misalignment, leading, words, trailing, offset, i, j;
	const unsigned long *wp;
	unsigned long wq;
	const unsigned char *p, *q;

	p = a;
	q = b;

	if (a == b || count == 0) {
		return 0;
	}

	/* less than one word: just compare the bytes */
	if (count < sizeof(unsigned long)) {
		for (i = 0; i < count; ++i) {
			if (p[i] != q[i]) {
				return p[i] - q[i];
			}
		}
		return 0;
	}

	/* compare leading bytes */
	misalignment = ((uintptr_t) p) % sizeof(unsigned long);
	leading = misalignment? sizeof(unsigned long) - misalignment : 0;

	for (i = 0; i < leading; ++i) {
		if (p[i] != q[i]) {
			return p[i] - q[i];
		}
	}

	/* compare aligned words from p, potentially unaligned loads from q */
	words = (count - leading) / sizeof(unsigned long);
	wp = (const unsigned long *) (p + leading);

	for (i = 0; i < words; ++i) {
		wq = lone_u64_read(q + leading + i * sizeof(unsigned long));
		if (wp[i] != wq) {
			/* found mismatched word, now find mismatched byte */
			offset = leading + i * sizeof(unsigned long);
			for (j = 0; j < sizeof(unsigned long); ++j) {
				if (p[offset + j] != q[offset + j]) {
					return p[offset + j] - q[offset + j];
				}
			}
		}
	}

	/* compare trailing bytes */
	offset = leading + words * sizeof(unsigned long);
	trailing = count - offset;

	for (i = 0; i < trailing; ++i) {
		if (p[offset + i] != q[offset + i]) {
			return p[offset + i] - q[offset + i];
		}
	}

	return 0;
}

bool lone_memory_is_equal(const void *a, const void *b, size_t count)
{
	return lone_memory_compare(a, b, count) == 0;
}

bool lone_memory_is_zero(const void *x, size_t count)
{
	size_t misalignment, leading, words, trailing, offset, i;
	const unsigned long *wp;
	const unsigned char *p;

	p = x;

	if (!x) {
		return true;
	}

	/* less than one word: just check it */
	if (count < sizeof(unsigned long)) {
		for (i = 0; i < count; ++i) {
			if (p[i]) { return false; }
		}
		return true;
	}

	/* check leading bytes */
	misalignment = ((uintptr_t) p) % sizeof(unsigned long);
	leading = misalignment? sizeof(unsigned long) - misalignment : 0;

	for (i = 0; i < leading; ++i) {
		if (p[i]) { return false; }
	}

	/* check aligned words */
	words = (count - leading) / sizeof(unsigned long);
	wp = (const unsigned long *) (p + leading);

	for (i = 0; i < words; ++i) {
		if (wp[i]) { return false; }
	}

	/* check trailing bytes */
	offset = leading + words * sizeof(unsigned long);
	trailing = count - offset;

	for (i = 0; i < trailing; ++i) {
		if (p[offset + i]) { return false; }
	}

	return true;
}

void lone_memory_move(const void *from, void *to, size_t count)
{
	size_t misalignment, leading, trailing, words, i, offset;
	unsigned long *aligned_destination;
	unsigned long word;
	const unsigned char *source;
	unsigned char *destination;

	source = from;
	destination = to;

	if (source == destination || count == 0) {
		return;
	}

	/* less than one word: just move the memory */
	if (count < sizeof(unsigned long)) {
		if (source >= destination) {
			/* destination is at or behind source, copy forwards */
			for (i = 0; i < count; ++i) {
				destination[i] = source[i];
			}
		} else {
			/* destination is ahead of source, copy backwards */
			for (i = count; i > 0; --i) {
				destination[i - 1] = source[i - 1];
			}
		}
		return;
	}

	if (source >= destination) {
		/* forward copy: leading bytes, aligned words, trailing bytes */

		/* move leading unaligned segment */
		misalignment = ((uintptr_t) destination) % sizeof(unsigned long);
		leading = misalignment? sizeof(unsigned long) - misalignment : 0;

		for (i = 0; i < leading; ++i) {
			destination[i] = source[i];
		}

		/* aligned word stores, potentially unaligned word loads */
		words               = (count - leading) / sizeof(unsigned long);
		aligned_destination = ((unsigned long *) (destination + leading));

		for (i = 0; i < words; ++i) {
			word = lone_u64_read(source + leading + i * sizeof(unsigned long));
			aligned_destination[i] = word;
		}

		offset = leading + words * sizeof(unsigned long);
		trailing = count - offset;

		for (i = 0; i < trailing; ++i) {
			destination[offset + i] = source[offset + i];
		}
	} else {
		/* backward copy: leading bytes, aligned words, trailing bytes */

		/* align the end of the destination for word stores */
		trailing = ((uintptr_t) (destination + count)) % sizeof(unsigned long);

		for (i = 0; i < trailing; ++i) {
			destination[count - 1 - i] = source[count - 1 - i];
		}

		/* aligned word stores, potentially unaligned word loads */
		words = (count - trailing) / sizeof(unsigned long);
		leading = (count - trailing) % sizeof(unsigned long);
		aligned_destination = ((unsigned long *) (destination + leading));

		for (i = words; i > 0; --i) {
			word = lone_u64_read(source + leading + (i - 1) * sizeof(unsigned long));
			aligned_destination[i - 1] = word;
		}

		for (i = leading; i > 0; --i) {
			destination[i - 1] = source[i - 1];
		}
	}
}

void lone_memory_set(void *to, unsigned char byte, size_t count)
{
	size_t misalignment, leading, trailing, words, i;
	unsigned char *destination;
	unsigned long *aligned, word;

	destination = to;

	/* less than one word: just fill it up */
	if (count < sizeof(unsigned long)) {
		for (i = 0; i < count; ++i) {
			destination[i] = byte;
		}
		return;
	}

	/* replicate byte across all lanes of the word
	 * 0xAB * 0x0101010101010101 = 0xABABABABABABABAB */
	word = ((unsigned long) byte) * (~0UL / 0xFF);

	/* fill leading bytes to realign pointer */
	misalignment = ((uintptr_t) destination) % sizeof(unsigned long);
	leading = misalignment? sizeof(unsigned long) - misalignment : 0;

	for (i = 0; i < leading; ++i) {
		destination[i] = byte;
	}

	/* aligned word fill */
	words = (count - leading) / sizeof(unsigned long);
	aligned = ((unsigned long *) (destination + leading));

	for (i = 0; i < words; ++i) {
		aligned[i] = word;
	}

	/* fill trailing bytes */
	trailing = (count - leading) % sizeof(unsigned long);

	for (i = 0; i < trailing; ++i) {
		destination[leading + words * sizeof(unsigned long) + i] = byte;
	}
}

void lone_memory_zero(void *to, size_t count)
{
	lone_memory_set(to, 0, count);
}

void lone_memory_one(void *to, size_t count)
{
	lone_memory_set(to, ~((unsigned char ) 0), count);
}

size_t lone_c_string_length(char *c_string)
{
	size_t length = 0;
	if (!c_string) { return 0; }
	while (c_string[length++]);
	return length - 1;
}

/* Compilers emit calls to mem* functions even with -nostdlib */
void *memset(void *to, int byte, size_t count)
{
	lone_memory_set(to, (unsigned char) byte, count);
	return to;
}

void *memcpy(void *to, const void *from, size_t count)
{
	lone_memory_move(from, to, count);
	return to;
}

void *memmove(void *to, const void *from, size_t count)
{
	memcpy(to, from, count);
	return to;
}
