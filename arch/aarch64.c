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

static long
system_call_0(long n)
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

static long
system_call_1(long n, long _1)
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

static long
system_call_2(long n, long _1, long _2)
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

static long
system_call_3(long n, long _1, long _2, long _3)
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

static long
system_call_4(long n, long _1, long _2, long _3, long _4)
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

static long
system_call_5(long n, long _1, long _2, long _3, long _4, long _5)
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

static long
system_call_6(long n, long _1, long _2, long _3, long _4, long _5, long _6)
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

/**
 * Registers may contain pointers to garbage collector roots.
 * They must be spilled onto the stack so that they can be marked.
 * Link register is the only architectural register, others are conventional.
 * Nearly all of arm64's registers may be used as scratch or result registers.
 * Probably best to just save all 30 of them just in case.
 **/
typedef long lone_registers[30];
extern void lone_save_registers(lone_registers);

__asm__
(

".global lone_save_registers"            "\n"
".type lone_save_registers,@function"    "\n"

"lone_save_registers:"                   "\n" // x0 = &lone_registers
"stp x0,  x1,  [x0, #0  ]"               "\n"
"stp x2,  x3,  [x0, #16 ]"               "\n"
"stp x4,  x5,  [x0, #32 ]"               "\n"
"stp x6,  x7,  [x0, #48 ]"               "\n"
"stp x8,  x9,  [x0, #64 ]"               "\n"
"stp x10, x11, [x0, #80 ]"               "\n"
"stp x12, x13, [x0, #96 ]"               "\n"
"stp x14, x15, [x0, #112]"               "\n"
"stp x16, x17, [x0, #128]"               "\n"
"stp x18, x19, [x0, #144]"               "\n"
"stp x20, x21, [x0, #160]"               "\n"
"stp x22, x23, [x0, #176]"               "\n"
"stp x24, x25, [x0, #192]"               "\n"
"stp x26, x27, [x0, #208]"               "\n"
"stp x28, x29, [x0, #224]"               "\n"
"ret"                                    "\n"

);

/**
 *
 * initial stack layout - logical
 *
 * sp → 0                         | argc
 *      1                         | argv
 *      argv + *argc + 1          | envp
 *      &(*envp++ == 0) + 1       | auxv
 *
 * initial stack layout - bytes
 *
 * sp → 0                         | argc
 *      8                         | argv
 *      argv + 8 * (*argc + 1)    | envp
 *      &(*envp++ == 0) + 8       | auxv
 *
 **/
__asm__
(

".global _start"                 "\n"  // place _start in the symbol table
"_start:"                        "\n"  // program entry point

                                       // compute argc, argv, envp and auxv
"ldr x0, [sp]"                   "\n"  // argc: x0 =   *sp
"add x1, sp, 8"                  "\n"  // argv: x1 =    sp + 8
"add x2, x0, 1"                  "\n"  //       x2 =  argc + 1
"lsl x2, x2, 3"                  "\n"  //       x2 =    x2 * 8
"add x2, x1, x2"                 "\n"  // envp: x2 =  argv + x2
"mov x3, x2"                     "\n"  //       x3 =  envp
"0:"                             "\n"  //       null finder loop:
"ldr x8, [x3], 8"                "\n"  //       x8 = *x3
                                       //       x3 =  x3 + 8
"cbnz x8, 0b"                    "\n"  //       goto loop if x8 != 0
                                       // auxv: x3

"and sp, x1, -16"                "\n"  // ensure 16 byte alignment

"bl lone"                        "\n"  // call lone; returns status code in x0

#define S2(s) #s
#define S(s) S2(s)

"mov x8, " S(__NR_exit)          "\n"  // ensure clean process termination
"svc 0"                          "\n"  // exit with returned status code

#undef S2
#undef S

);
