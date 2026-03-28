/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/system.h>
#include <lone/memory.h>
#include <lone/hash.h>
#include <lone/auxiliary_vector.h>

void lone_system_initialize(struct lone_system *system, struct lone_auxiliary_vector *auxiliary_vector, struct lone_bytes initial_static_memory
)
{
	struct lone_bytes random_bytes;
	size_t page_size;

	random_bytes = lone_auxiliary_vector_random(auxiliary_vector);
	page_size = lone_auxiliary_vector_page_size(auxiliary_vector);

	lone_memory_initialize(system, page_size, initial_static_memory);
	lone_hash_initialize(system, random_bytes);
}
