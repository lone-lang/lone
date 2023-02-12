/**
 *
 * architecture:    aarch64
 * register-size:   64 bits
 * stack-alignment: 16 bytes
 * system-call:     x0 = "svc 0" [x8] x0 x1 x2 x3 x4 x5
 *
 **/

long system_call(long n, long _1, long _2, long _3, long _4, long _5, long _6)
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
		: "r" (x1), "r" (x2), "r" (x3),
		  "r" (x4), "r" (x5), "r" (x8)
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
