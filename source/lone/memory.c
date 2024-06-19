/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/memory.h>

void lone_memory_initialize(struct lone_system *system, struct lone_bytes initial_static_memory)
{
	system->memory = (struct lone_memory *) __builtin_assume_aligned(initial_static_memory.pointer, LONE_ALIGNMENT);
	system->memory->prev = system->memory->next = 0;
	system->memory->free = 1;
	system->memory->size = initial_static_memory.count - sizeof(struct lone_memory);
}
