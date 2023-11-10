#ifndef LONE_MEMORY_FUNCTIONS_HEADER
#define LONE_MEMORY_FUNCTIONS_HEADER

#include <lone/types.h>

void lone_memory_move(void *from, void *to, size_t count);
void lone_memory_set(void *to, unsigned char byte, size_t count);
size_t lone_c_string_length(char *c_string);

#endif /* LONE_MEMORY_FUNCTIONS_HEADER */
