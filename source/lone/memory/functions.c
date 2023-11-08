#include <lone/memory/functions.h>

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

size_t lone_c_string_length(char *c_string)
{
	size_t length = 0;
	if (!c_string) { return 0; }
	while (c_string[length++]);
	return length - 1;
}
