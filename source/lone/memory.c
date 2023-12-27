/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/memory.h>
#include <lone/memory/heap.h>

void lone_memory_initialize(struct lone_lisp *lone, struct lone_bytes memory, void *stack)
{
	lone->memory.stack = stack;

	lone->memory.general = (struct lone_memory *) __builtin_assume_aligned(memory.pointer, LONE_ALIGNMENT);
	lone->memory.general->prev = lone->memory.general->next = 0;
	lone->memory.general->free = 1;
	lone->memory.general->size = memory.count - sizeof(struct lone_memory);

	lone_heap_initialize(lone);
}
