/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/modules.h>
#include <lone/modules/intrinsic.h>

#include <lone/modules/linux.h>
#include <lone/modules/lone.h>
#include <lone/modules/math.h>
#include <lone/modules/text.h>
#include <lone/modules/list.h>

void lone_modules_intrinsic_initialize(struct lone_lisp *lone, int argc, char **argv, char **envp, struct auxiliary *auxv)
{
	lone_module_linux_initialize(lone, argc, argv, envp, auxv);
	lone_module_lone_initialize(lone);
	lone_module_math_initialize(lone);
	lone_module_text_initialize(lone);
	lone_module_list_initialize(lone);
}
