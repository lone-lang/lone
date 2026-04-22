/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/compiler/stack_protector.h>
#include <lone/types.h>
#include <lone/linux.h>

unsigned long __stack_chk_guard;

void __stack_chk_fail(void)
{
	linux_exit(-1);
}

void
__attribute__((no_stack_protector))
lone_compiler_stack_protector_initialize(struct lone_bytes random)
{
	unsigned char fallback[8];

	if (random.pointer == 0) {
		/* kernel did not provide AT_RANDOM
		 * read bytes from /dev/urandom instead
		 * exit if that fails since the canary
		 * cannot be initialized safely and a
		 * predictable stack canary is pointless */
		random = LONE_BYTES_FROM_ARRAY(fallback);
		if (linux_dev_urandom(random) < 0) { linux_exit(-1); }
	}

	__stack_chk_guard = lone_u64_read(random.pointer);
}
