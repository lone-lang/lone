/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone.h>
#include <lone/start.h>
#include <lone/auxiliary_vector.h>
#include <lone/compiler/stack_protector.h>

long
__attribute__((no_stack_protector))
lone_start(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv)
{
#ifdef LONE_COMPILER_STACK_PROTECTOR
	lone_compiler_stack_protector_initialize(auxv);
#endif
	return lone(argc, argv, envp, auxv);
}
