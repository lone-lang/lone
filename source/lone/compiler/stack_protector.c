/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/compiler/stack_protector.h>
#include <lone/types.h>
#include <lone/linux.h>

unsigned long __stack_chk_guard;

void __stack_chk_fail(void)
{
	linux_exit(-1);
}

void lone_compiler_stack_protector_initialize(struct lone_bytes random)
{
	__stack_chk_guard = lone_u64_read(random.pointer);
}
