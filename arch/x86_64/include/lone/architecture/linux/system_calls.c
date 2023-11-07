/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <linux/unistd.h>

/**
 *
 * architecture:    x86_64
 * register-size:   64 bits
 * stack-alignment: 16 bytes
 * system-call:     rax = "syscall" [rax] rdi rsi rdx r10 r8 r9
 * clobbers:        rcx r11
 **/

static long system_call_0(long number)
{
	register long rax __asm__("rax") = number;

	__asm__ volatile
	("syscall"

		: "=r" (rax)
		:  "r" (rax)
		: "rcx", "r11", "cc", "memory");

	return rax;
}

static long system_call_1(long number, long _1)
{
	register long rax __asm__("rax") = number;
	register long rdi __asm__("rdi") = _1;

	__asm__ volatile
	("syscall"

		: "+r" (rax)
		: "r" (rdi)
		: "rcx", "r11", "cc", "memory");

	return rax;
}

static long system_call_2(long number, long _1, long _2)
{
	register long rax __asm__("rax") = number;
	register long rdi __asm__("rdi") = _1;
	register long rsi __asm__("rsi") = _2;

	__asm__ volatile
	("syscall"

		: "+r" (rax)
		: "r" (rdi), "r" (rsi)
		: "rcx", "r11", "cc", "memory");

	return rax;
}

static long system_call_3(long number, long _1, long _2, long _3)
{
	register long rax __asm__("rax") = number;
	register long rdi __asm__("rdi") = _1;
	register long rsi __asm__("rsi") = _2;
	register long rdx __asm__("rdx") = _3;

	__asm__ volatile
	("syscall"

		: "+r" (rax)
		: "r" (rdi), "r" (rsi), "r" (rdx)
		: "rcx", "r11", "cc", "memory");

	return rax;
}

static long system_call_4(long number, long _1, long _2, long _3, long _4)
{
	register long rax __asm__("rax") = number;
	register long rdi __asm__("rdi") = _1;
	register long rsi __asm__("rsi") = _2;
	register long rdx __asm__("rdx") = _3;
	register long r10 __asm__("r10") = _4;

	/* r10 may be clobbered but can't be in the clobbers list
	   because the compiler won't use clobbered registers as inputs.
	   So they're placed in the outputs list instead. */
	__asm__ volatile
	("syscall"

		: "+r" (rax), "+r" (r10)
		: "r" (rdi), "r" (rsi), "r" (rdx)
		: "rcx", "r11", "cc", "memory");

	return rax;
}

static long system_call_5(long number, long _1, long _2, long _3, long _4, long _5)
{
	register long rax __asm__("rax") = number;
	register long rdi __asm__("rdi") = _1;
	register long rsi __asm__("rsi") = _2;
	register long rdx __asm__("rdx") = _3;
	register long r10 __asm__("r10") = _4;
	register long r8  __asm__("r8")  = _5;

	/* r8 and r10 may be clobbered but can't be in the clobbers list
	   because the compiler won't use clobbered registers as inputs.
	   So they're placed in the outputs list instead. */
	__asm__ volatile
	("syscall"

		: "+r" (rax), "+r" (r8), "+r" (r10)
		: "r" (rdi), "r" (rsi), "r" (rdx)
		: "rcx", "r11", "cc", "memory");

	return rax;
}

static long system_call_6(long number, long _1, long _2, long _3, long _4, long _5, long _6)
{
	register long rax __asm__("rax") = number;
	register long rdi __asm__("rdi") = _1;
	register long rsi __asm__("rsi") = _2;
	register long rdx __asm__("rdx") = _3;
	register long r10 __asm__("r10") = _4;
	register long r8  __asm__("r8")  = _5;
	register long r9  __asm__("r9")  = _6;

	/* r8, r9 and r10 may be clobbered but can't be in the clobbers list
	   because the compiler won't use clobbered registers as inputs.
	   So they're placed in the outputs list instead. */
	__asm__ volatile
	("syscall"

		: "+r" (rax),
		  "+r" (r8), "+r" (r9), "+r" (r10)
		: "r" (rdi), "r" (rsi), "r" (rdx)
		: "rcx", "r11", "cc", "memory");

	return rax;
}
