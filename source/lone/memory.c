/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/memory.h>

void lone_memory_initialize(struct lone_system *system, size_t page_size)
{
	system->allocator = (struct lone_memory_allocator) { 0 };
	system->allocator.page_size = page_size;
}
