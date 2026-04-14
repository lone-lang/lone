/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/random.h>
#include <lone/auxiliary_vector.h>
#include <lone/memory/functions.h>
#include <lone/linux.h>

void lone_random_with_urandom_fallback(
		struct lone_auxiliary_vector *auxiliary_vector,
		struct lone_bytes buffer)
{
	struct lone_bytes random;

	random = lone_auxiliary_vector_random(auxiliary_vector);

	if (random.pointer) {
		if (buffer.count <= 16) {
			lone_memory_move(random.pointer, buffer.pointer, buffer.count);
		} else {
			lone_memory_move(random.pointer, buffer.pointer, 16);
			buffer.pointer += 16;
			buffer.count   -= 16;
			if (linux_dev_urandom(buffer) < 0) { linux_exit(-1); }
		}
	} else {
		if (linux_dev_urandom(buffer) < 0) { linux_exit(-1); }
	}
}
