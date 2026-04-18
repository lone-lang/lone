/* SPDX-License-Identifier: AGPL-3.0-or-later */

#ifndef LONE_COMPILER_STACK_PROTECTOR_HEADER
#define LONE_COMPILER_STACK_PROTECTOR_HEADER

extern unsigned long __stack_chk_guard;

void
__attribute__((noreturn))
__stack_chk_fail(void);

#endif /* LONE_COMPILER_STACK_PROTECTOR_HEADER */
