/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_COMPILER_STACK_PROTECTOR_HEADER
#define LONE_COMPILER_STACK_PROTECTOR_HEADER

#include <lone/types.h>

extern unsigned long __stack_chk_guard;

void
__attribute__((noreturn))
__stack_chk_fail(void);

void lone_compiler_stack_protector_initialize(struct lone_bytes random);

#endif /* LONE_COMPILER_STACK_PROTECTOR_HEADER */
