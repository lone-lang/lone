/* SPDX-License-Identifier: AGPL-3.0-or-later */

/* ╭─────────────────────────────┨ LONE LISP ┠──────────────────────────────╮
   │                                                                        │
   │                       The standalone Linux Lisp                        │
   │                                                                        │
   ╰────────────────────────────────────────────────────────────────────────╯ */

#include <lone.h>

#include <lone/system.h>
#include <lone/auxiliary_vector.h>

#include <lone/lisp.h>
#include <lone/lisp/definitions.h>
#include <lone/lisp/types.h>

#include <lone/lisp/module.h>
#include <lone/lisp/modules/intrinsic.h>
#include <lone/lisp/modules/embedded.h>

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
	static unsigned char __attribute__((aligned(LONE_ALIGNMENT))) bytes[LONE_LISP_MEMORY_SIZE];
	struct lone_bytes memory = { sizeof(bytes), bytes }, random = lone_auxiliary_vector_random(auxv);
	struct lone_system system;
	struct lone_lisp lone;

	lone_system_initialize(&system, memory, random);
	lone_lisp_initialize(&lone, &system, stack);

	lone_lisp_modules_intrinsic_initialize(&lone, argc, argv, envp, auxv);

	lone_lisp_module_path_push_all(&lone, 4,

		".",
		"~/.lone/modules",
		"~/.local/lib/lone/modules",
		"/usr/lib/lone/modules"

	);

	lone_lisp_modules_embedded_load(&lone, lone_auxiliary_vector_embedded_segment(auxv));

	lone_lisp_module_load_null_from_standard_input(&lone);

	return 0;
}
