/* SPDX-License-Identifier: AGPL-3.0-or-later */

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
