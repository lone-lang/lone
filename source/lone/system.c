/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/system.h>
#include <lone/memory.h>
#include <lone/random.h>
#include <lone/auxiliary_vector.h>

void lone_system_initialize(struct lone_system *system, struct lone_auxiliary_vector *auxiliary_vector)
{
	size_t page_size;

	page_size = lone_auxiliary_vector_page_size(auxiliary_vector);

	lone_memory_initialize(system, page_size);
	lone_random_with_urandom_fallback(auxiliary_vector, LONE_BYTES_VALUE_FROM_ARRAY(system->random));
}
