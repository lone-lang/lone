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

long system_call_0(long number)
{
	register long rax __asm__("rax") = number;

	__asm__ volatile
	("syscall"

		: "=r" (rax)
		:  "r" (rax)
		: "rcx", "r11", "cc", "memory");

	return rax;
}

long system_call_1(long number, long _1)
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

long system_call_2(long number, long _1, long _2)
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

long system_call_3(long number, long _1, long _2, long _3)
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

long system_call_4(long number, long _1, long _2, long _3, long _4)
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

long system_call_5(long number, long _1, long _2, long _3, long _4, long _5)
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

long system_call_6(long number, long _1, long _2, long _3, long _4, long _5, long _6)
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

/**
 * Registers may contain pointers to garbage collector roots.
 * They must be spilled onto the stack so that they can be marked.
 * The x86_64 provides 16 general purpose registers.
 **/
typedef long lone_registers[16];
extern void lone_save_registers(lone_registers);

__asm__
(

".global lone_save_registers"            "\n"
".type lone_save_registers,@function"    "\n"

"lone_save_registers:"                   "\n" // rdi = &lone_registers
"mov %rax,   0(%rdi)"                    "\n"
"mov %rbx,   8(%rdi)"                    "\n"
"mov %rcx,  16(%rdi)"                    "\n"
"mov %rdx,  24(%rdi)"                    "\n"
"mov %rsp,  32(%rdi)"                    "\n"
"mov %rbp,  40(%rdi)"                    "\n"
"mov %rsi,  48(%rdi)"                    "\n"
"mov %rdi,  56(%rdi)"                    "\n"
"mov %r8,   64(%rdi)"                    "\n"
"mov %r9,   72(%rdi)"                    "\n"
"mov %r10,  80(%rdi)"                    "\n"
"mov %r11,  88(%rdi)"                    "\n"
"mov %r12,  96(%rdi)"                    "\n"
"mov %r13, 104(%rdi)"                    "\n"
"mov %r14, 112(%rdi)"                    "\n"
"mov %r15, 120(%rdi)"                    "\n"
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
 *      argv + (*argc * 8) + 8    | envp
 *      &(*envp++ == 0) + 8       | auxv
 *
 **/
__asm__
(

".global _start"                 "\n"  // place _start in the symbol table
"_start:"                        "\n"  // program entry point

                                       // compute argc, argv, envp and auxv

"pop %rdi"                       "\n"  // argc: rdi = pop
"mov %rsp, %rsi"                 "\n"  // argv: rsi = sp
"lea 8(%rsi, %rdi, 8), %rdx"     "\n"  // envp: rdx = rsi + (rdi * 8) + 8

"lea 0(%rdx), %rcx"              "\n"  //       rcx = rdx
"0:"                             "\n"  // loop:
"add $8, %rcx"                   "\n"  //       rcx = rcx + 9
"cmp $0, -8(%rcx)"               "\n"  //       *(rcx - 8) == 0 ?
"jnz 0b"                         "\n"  //       loop if not zero
                                       //       rcx - 8 == 0
                                       // auxv: rcx

                                       // x86_64 SysV ABI requirements:
"xor %rbp, %rbp"                 "\n"  // zero the deepest stack frame
"and $-16, %rsp"                 "\n"  // ensure 16 byte stack alignment

"call lone"                      "\n"  // call lone
"mov %rax, %rdi"                 "\n"  // status code returned in rax

#define S2(s) #s
#define S(s) S2(s)

"mov $" S(__NR_exit) ", %rax"    "\n"  // ensure clean process termination
"syscall"                        "\n"  // exit with returned status code

#undef S2
#undef S

);
