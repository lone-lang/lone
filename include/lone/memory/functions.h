/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_MEMORY_FUNCTIONS_HEADER
#define LONE_MEMORY_FUNCTIONS_HEADER

#include <lone/types.h>

int lone_memory_compare(unsigned char *a, unsigned char *b, size_t count);
bool lone_memory_is_equal(unsigned char *a, unsigned char *b, size_t count);
void lone_memory_move(void *from, void *to, size_t count);
void lone_memory_set(void *to, unsigned char byte, size_t count);
void lone_memory_zero(void *to, size_t count);
size_t lone_c_string_length(char *c_string);

#endif /* LONE_MEMORY_FUNCTIONS_HEADER */
