/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <linux/unistd.h>

/**
 *
 * architecture:    x86_64
 * register-size:   64 bits
 * stack-alignment: 16 bytes
 * system-call:     rax = "syscall" [rax] rdi rsi rdx r10 r8 r9
 * clobbers:        rcx r11
 *
 * The syscall instruction saves rip to rcx and rflags to r11.
 * The kernel saves all other registers to pt_regs on entry
 * and restores them on return. Argument registers are preserved.
 *
 * References:
 *     Intel:
 *         SDM Volume 2B, SYSCALL instruction     saves rip to rcx, rflags to r11
 *         https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
 *     AMD:
 *         APM Volume 3, §4.8.8 SYSCALL/SYSRET    saves rip to rcx, rflags to r11
 *         https://www.amd.com/en/support/tech-docs/amd64-architecture-programmers-manual-volumes-1-5
 *     Linux:
 *         arch/x86/entry/entry_64.S              lines 49-85
 *         tools/include/nolibc/arch-x86.h        lines 184-190
 *         man 2 syscall                          architecture calling conventions
 *
 **/

long linux_system_call_0(long number)
{
	register long rax __asm__("rax") = number;

	__asm__ volatile
	("syscall"

		: "+r" (rax)
		:
		: "rcx", "r11", "cc", "memory");

	return rax;
}

long linux_system_call_1(long number, long _1)
{
	register long rax __asm__("rax") = number;
	register long rdi __asm__("rdi") = _1;

	__asm__ volatile
	("syscall"

		: "+r" (rax)
		:  "r" (rdi)
		: "rcx", "r11", "cc", "memory");

	return rax;
}

long linux_system_call_2(long number, long _1, long _2)
{
	register long rax __asm__("rax") = number;
	register long rdi __asm__("rdi") = _1;
	register long rsi __asm__("rsi") = _2;

	__asm__ volatile
	("syscall"

		: "+r" (rax)
		:  "r" (rdi), "r" (rsi)
		: "rcx", "r11", "cc", "memory");

	return rax;
}

long linux_system_call_3(long number, long _1, long _2, long _3)
{
	register long rax __asm__("rax") = number;
	register long rdi __asm__("rdi") = _1;
	register long rsi __asm__("rsi") = _2;
	register long rdx __asm__("rdx") = _3;

	__asm__ volatile
	("syscall"

		: "+r" (rax)
		:  "r" (rdi), "r" (rsi), "r" (rdx)
		: "rcx", "r11", "cc", "memory");

	return rax;
}

long linux_system_call_4(long number, long _1, long _2, long _3, long _4)
{
	register long rax __asm__("rax") = number;
	register long rdi __asm__("rdi") = _1;
	register long rsi __asm__("rsi") = _2;
	register long rdx __asm__("rdx") = _3;
	register long r10 __asm__("r10") = _4;

	__asm__ volatile
	("syscall"

		: "+r" (rax)
		:  "r" (rdi), "r" (rsi), "r" (rdx), "r" (r10)
		: "rcx", "r11", "cc", "memory");

	return rax;
}

long linux_system_call_5(long number, long _1, long _2, long _3, long _4, long _5)
{
	register long rax __asm__("rax") = number;
	register long rdi __asm__("rdi") = _1;
	register long rsi __asm__("rsi") = _2;
	register long rdx __asm__("rdx") = _3;
	register long r10 __asm__("r10") = _4;
	register long r8  __asm__("r8")  = _5;

	__asm__ volatile
	("syscall"

		: "+r" (rax)
		:  "r" (rdi), "r" (rsi), "r" (rdx), "r" (r10),
		   "r" (r8)
		: "rcx", "r11", "cc", "memory");

	return rax;
}

long linux_system_call_6(long number, long _1, long _2, long _3, long _4, long _5, long _6)
{
	register long rax __asm__("rax") = number;
	register long rdi __asm__("rdi") = _1;
	register long rsi __asm__("rsi") = _2;
	register long rdx __asm__("rdx") = _3;
	register long r10 __asm__("r10") = _4;
	register long r8  __asm__("r8")  = _5;
	register long r9  __asm__("r9")  = _6;

	__asm__ volatile
	("syscall"

		: "+r" (rax)
		:  "r" (rdi), "r" (rsi), "r" (rdx), "r" (r10),
		   "r" (r8),  "r" (r9)
		: "rcx", "r11", "cc", "memory");

	return rax;
}
