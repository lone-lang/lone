/* SPDX-License-Identifier: AGPL-3.0-or-later */

/* ╭─────────────────────────────┨ LONE LISP ┠──────────────────────────────╮
   │                                                                        │
   │                       The standalone Linux Lisp                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */
#include <lone/definitions.h>
#include <lone/types.h>
#include <lone/lisp.h>
#include <lone/modules.h>
#include <lone/modules/intrinsic.h>
#include <lone/modules/embedded.h>
#include <lone/utilities.h>

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

long lone(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv)
{
	void *stack = __builtin_frame_address(0);
	static unsigned char __attribute__((aligned(LONE_ALIGNMENT))) bytes[LONE_MEMORY_SIZE];
	struct lone_bytes memory = { sizeof(bytes), bytes }, random = lone_auxiliary_vector_random(auxv);
	struct lone_lisp lone;

	lone_lisp_initialize(&lone, memory, 1024, stack, random);
	lone_modules_intrinsic_initialize(&lone, argc, argv, envp, auxv);
	lone_module_path_push_all(&lone, 4,

		".",
		"~/.lone/modules",
		"~/.local/lib/lone/modules",
		"/usr/lib/lone/modules"

	);

	lone_modules_embedded_load(&lone, auxv);

	lone_module_load_null_from_standard_input(&lone);

	return 0;
}
