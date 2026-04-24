/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <linux/unistd.h>

#include <lone/start.h>

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

/* Function attributes:
 *
 * 	naked                 suppresses compiler-emitted prologue and epilogue
 * 	                      the asm body is the entire function
 *
 * 	externally_visible    stops -flto from internalizing the symbol
 * 	                      under -fvisibility=hidden so the ELF entry
 * 	                      point reference still resolves
 *
 * 	used, retain          stops the optimizer and linker from dropping
 * 	                      the function's code and section respectively
 * 	                      when they find that no C code calls lone_entry
 */
__attribute__((naked, externally_visible, used, retain))
void lone_entry(void)
{
	__asm__
	(
	                                       // compute argc, argv, envp and auxv
	"ldr x0, [sp]"                   "\n"  // argc: x0 =   *sp
	"add x1, sp, #8"                 "\n"  // argv: x1 =    sp + 8
	"add x2, x0, #1"                 "\n"  //       x2 =  argc + 1
	"lsl x2, x2, #3"                 "\n"  //       x2 =    x2 * 8
	"add x2, x1, x2"                 "\n"  // envp: x2 =  argv + x2
	"mov x3, x2"                     "\n"  //       x3 =  envp
	"0:"                             "\n"  //       null finder loop:
	"ldr x8, [x3], #8"               "\n"  //       x8 = *x3
	                                       //       x3 =  x3 + 8
	"cbnz x8, 0b"                    "\n"  //       goto loop if x8 != 0
	                                       // auxv: x3

	                                       // aarch64 AAPCS64 requirements:
	                                       //
	"mov x29, #0"                    "\n"  // zero the deepest stack frame
	                                       //
	                                       // AND (immediate) forbids sp as the
	                                       // source register; stage sp through
	                                       // x8 so the AND reads from a GP
	                                       // register and writes back to sp.
	"mov x8, sp"                     "\n"  // x8 = sp
	"and sp, x8, #-16"               "\n"  // ensure 16 byte alignment

	"bl lone_start"                  "\n"  // call lone_start; returns status code in x0

#define S2(s) #s
#define S(s) S2(s)

	"mov x8, #" S(__NR_exit)         "\n"  // ensure clean process termination
	"svc #0"                         "\n"  // exit with returned status code

#undef S2
#undef S

	);
}
