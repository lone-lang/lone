/* SPDX-License-Identifier: AGPL-3.0-or-later */

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
