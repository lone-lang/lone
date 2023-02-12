#include <linux/unistd.h>

/**
 *
 * architecture:    aarch64
 * register-size:   64 bits
 * stack-alignment: 16 bytes
 * system-call:     x0 = "svc 0" [x8] x0 x1 x2 x3 x4 x5
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
