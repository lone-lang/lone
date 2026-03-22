/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/memory/functions.h>

int lone_memory_compare(void *a, void *b, size_t count)
{
	size_t i;
	unsigned char *p, *q, x, y;

	if (a == b || count == 0) {
		return 0;
	}

	p = a;
	q = b;

	for (i = 0; i < count; ++i) {
		x = p[i];
		y = q[i];

		if (x == y) {
			continue;
		}

		return x - y;
	}

	return 0;
}

bool lone_memory_is_equal(void *a, void *b, size_t count)
{
	return lone_memory_compare(a, b, count) == 0;
}

bool lone_memory_is_zero(void *x, size_t count)
{
	unsigned char *p;
	lone_size i;

	if (!x) {
		return true;
	}

	for (i = 0, p = x; i < count; ++i) {
		if (p[i]) { return false; }
	}

	return true;
}

void lone_memory_move(void *from, void *to, size_t count)
{
	unsigned char *source = from, *destination = to;

	if (source >= destination) {
		/* destination is at or behind source, copy forwards */
		while (count--) { *destination++ = *source++; }
	} else {
		/* destination is ahead of source, copy backwards */
		source += count; destination += count;
		while (count--) { *--destination = *--source; }
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

void *memcpy(void *to, void *from, size_t count)
{
	lone_memory_move(from, to, count);
	return to;
}
