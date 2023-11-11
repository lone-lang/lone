/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <linux/unistd.h>

/**
 *
 * architecture:    aarch64
 * register-size:   64 bits
 * stack-alignment: 16 bytes
 * system-call:     x0 = "svc 0" [x8] x0 x1 x2 x3 x4 x5
 *
 * https://github.com/ARM-software/abi-aa
 * https://github.com/ARM-software/abi-aa/blob/main/aapcs64/aapcs64.rst
 *
 **/

long linux_system_call_0(long n)
{
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0");

	__asm__ volatile
	("svc 0"

		: "=r" (x0)
		:  "r" (x8)
		: "cc", "memory");

	return x0;
}

long linux_system_call_1(long n, long _1)
{
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = _1;

	__asm__ volatile
	("svc 0"

		: "+r" (x0)
		:  "r" (x8)
		: "cc", "memory");

	return x0;
}

long linux_system_call_2(long n, long _1, long _2)
{
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = _1;
	register long x1 __asm__("x1") = _2;

	__asm__ volatile
	("svc 0"

		: "+r" (x0)
		: "r" (x1),
		  "r" (x8)
		: "cc", "memory");

	return x0;
}

long linux_system_call_3(long n, long _1, long _2, long _3)
{
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = _1;
	register long x1 __asm__("x1") = _2;
	register long x2 __asm__("x2") = _3;

	__asm__ volatile
	("svc 0"

		: "+r" (x0)
		: "r" (x1), "r" (x2),
		  "r" (x8)
		: "cc", "memory");

	return x0;
}

long linux_system_call_4(long n, long _1, long _2, long _3, long _4)
{
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = _1;
	register long x1 __asm__("x1") = _2;
	register long x2 __asm__("x2") = _3;
	register long x3 __asm__("x3") = _4;

	__asm__ volatile
	("svc 0"

		: "+r" (x0)
		: "r" (x1), "r" (x2), "r" (x3),
		  "r" (x8)
		: "cc", "memory");

	return x0;
}

long linux_system_call_5(long n, long _1, long _2, long _3, long _4, long _5)
{
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = _1;
	register long x1 __asm__("x1") = _2;
	register long x2 __asm__("x2") = _3;
	register long x3 __asm__("x3") = _4;
	register long x4 __asm__("x4") = _5;

	__asm__ volatile
	("svc 0"

		: "+r" (x0)
		: "r" (x1), "r" (x2), "r" (x3), "r" (x4),
		  "r" (x8)
		: "cc", "memory");

	return x0;
}

long linux_system_call_6(long n, long _1, long _2, long _3, long _4, long _5, long _6)
{
	register long x8 __asm__("x8") = n;
	register long x0 __asm__("x0") = _1;
	register long x1 __asm__("x1") = _2;
	register long x2 __asm__("x2") = _3;
	register long x3 __asm__("x3") = _4;
	register long x4 __asm__("x4") = _5;
	register long x5 __asm__("x5") = _6;

	__asm__ volatile
	("svc 0"

		: "+r" (x0)
		: "r" (x1), "r" (x2), "r" (x3), "r" (x4), "r" (x5),
		  "r" (x8)
		: "cc", "memory");

	return x0;
}
