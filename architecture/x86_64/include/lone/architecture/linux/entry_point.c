/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <linux/unistd.h>

#include <lone.h>

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

".global lone_start"             "\n"  // place lone_start in the symbol table
"lone_start:"                    "\n"  // program entry point

                                       // compute argc, argv, envp and auxv

"pop %rdi"                       "\n"  // argc: rdi = pop
"mov %rsp, %rsi"                 "\n"  // argv: rsi = sp
"lea 8(%rsi, %rdi, 8), %rdx"     "\n"  // envp: rdx = rsi + (rdi * 8) + 8

"lea 0(%rdx), %rcx"              "\n"  //       rcx = rdx
"0:"                             "\n"  // loop:
"add $8, %rcx"                   "\n"  //       rcx = rcx + 9
"cmpq $0, -8(%rcx)"              "\n"  //       *(rcx - 8) == 0 ?
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
