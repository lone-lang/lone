/* SPDX-License-Identifier: AGPL-3.0-or-later */

/* ╭─────────────────────────────┨ LONE LISP ┠──────────────────────────────╮
   │                                                                        │
   │                       The standalone Linux Lisp                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#include <stdint.h>

#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/structures.h>
#include <lone/constants.h>
#include <lone/hash.h>
#include <lone/value.h>
#include <lone/value/module.h>
#include <lone/value/function.h>
#include <lone/value/primitive.h>
#include <lone/value/bytes.h>
#include <lone/value/text.h>
#include <lone/value/symbol.h>
#include <lone/value/list.h>
#include <lone/value/vector.h>
#include <lone/value/table.h>
#include <lone/value/integer.h>
#include <lone/value/pointer.h>
#include <lone/memory.h>
#include <lone/linux.h>
#include <lone/lisp.h>
#include <lone/lisp/reader.h>
#include <lone/lisp/evaluator.h>
#include <lone/lisp/printer.h>
#include <lone/utilities.h>
#include <lone/modules.h>
#include <lone/modules/lone.h>
#include <lone/modules/math.h>
#include <lone/modules/list.h>
#include <lone/modules/text.h>
#include <lone/modules/linux.h>

/* ╭─────────────────────────┨ LONE LISP MODULES ┠──────────────────────────╮
   │                                                                        │
   │    Built-in modules containing essential functionality.                │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
static void lone_modules_initialize(struct lone_lisp *lone, int argc, char **argv, char **envp, struct auxiliary *auxv)
{
	lone_module_linux_initialize(lone, argc, argv, envp, auxv);
	lone_module_lone_initialize(lone);
	lone_module_math_initialize(lone);
	lone_module_text_initialize(lone);
	lone_module_list_initialize(lone);

	lone_vector_push_all(lone, lone->modules.path, 4,

		lone_text_create_from_c_string(lone, "."),
		lone_text_create_from_c_string(lone, "~/.lone/modules"),
		lone_text_create_from_c_string(lone, "~/.local/lib/lone/modules"),
		lone_text_create_from_c_string(lone, "/usr/lib/lone/modules")

	);
}

/* ╭───────────────────────┨ LONE LISP ENTRY POINT ┠────────────────────────╮
   │                                                                        │
   │    Linux places argument, environment and auxiliary value arrays       │
   │    on the stack before jumping to the entry point of the process.      │
   │    Architecture-specific code collects this data and passes it to      │
   │    the lone function which begins execution of the lisp code.          │
   │                                                                        │
   │    During early initialization, lone has no dynamic memory             │
   │    allocation capabilities and so this function statically             │
   │    allocates 64 KiB of memory for the early bootstrapping process.     │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

#include <lone/architecture/linux/entry_point.c>

long lone(int argc, char **argv, char **envp, struct auxiliary *auxv)
{
	void *stack = __builtin_frame_address(0);
	static unsigned char __attribute__((aligned(LONE_ALIGNMENT))) bytes[LONE_MEMORY_SIZE];
	struct lone_bytes memory = { sizeof(bytes), bytes }, random = lone_get_auxiliary_random(auxv);
	struct lone_lisp lone;

	lone_lisp_initialize(&lone, memory, 1024, stack, random);
	lone_modules_initialize(&lone, argc, argv, envp, auxv);

	lone_module_load_null_from_standard_input(&lone);

	return 0;
}
