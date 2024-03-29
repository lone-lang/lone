/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/memory/functions.h>

int lone_memory_compare(unsigned char *a, unsigned char *b, size_t count)
{
	size_t i;
	unsigned char x, y;

	if (a == b || count == 0) {
		return 0;
	}

	for (i = 0; i < count; ++i) {
		x = a[i];
		y = b[i];

		if (x == y) {
			continue;
		}

		return x - y;
	}

	return 0;
}

bool lone_memory_is_equal(unsigned char *a, unsigned char *b, size_t count)
{
	return lone_memory_compare(a, b, count) == 0;
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
	unsigned char *memory = to;
	size_t i;

	for (i = 0; i < count; ++i) {
		memory[i] = byte;
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
