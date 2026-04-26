/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/compiler/stack_protector.h>
#include <lone/random.h>
#include <lone/types.h>
#include <lone/linux.h>

unsigned long __stack_chk_guard;

void __stack_chk_fail(void)
{
	linux_exit(-1);
}

void
__attribute__((no_stack_protector))
lone_compiler_stack_protector_initialize(struct lone_auxiliary_vector *auxiliary_vector)
{
	unsigned long guard;

	lone_random_with_urandom_fallback(
		auxiliary_vector,
		LONE_BYTES_VALUE(sizeof(guard), &guard)
	);

	/* this guard can only be assigned
	   in a no_stack_protector function */
	__stack_chk_guard = guard;
}
