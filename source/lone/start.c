/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone.h>
#include <lone/start.h>
#include <lone/auxiliary_vector.h>
#include <lone/compiler/stack_protector.h>

long
__attribute__((no_stack_protector))
lone_start(int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv)
{
	lone_compiler_stack_protector_initialize(lone_auxiliary_vector_random(auxv));
	return lone(argc, argv, envp, auxv);
}
