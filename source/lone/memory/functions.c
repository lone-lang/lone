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
