/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/modules.h>
#include <lone/modules/intrinsic.h>

#include <lone/modules/intrinsic/linux.h>
#include <lone/modules/intrinsic/lone.h>
#include <lone/modules/intrinsic/math.h>
#include <lone/modules/intrinsic/bytes.h>
#include <lone/modules/intrinsic/text.h>
#include <lone/modules/intrinsic/list.h>
#include <lone/modules/intrinsic/vector.h>
#include <lone/modules/intrinsic/table.h>

void lone_modules_intrinsic_initialize(struct lone_lisp *lone, int argc, char **argv, char **envp, struct lone_auxiliary_vector *auxv)
{
	lone_modules_intrinsic_linux_initialize(lone, argc, argv, envp, auxv);
	lone_modules_intrinsic_lone_initialize(lone);
	lone_modules_intrinsic_math_initialize(lone);
	lone_modules_intrinsic_bytes_initialize(lone);
	lone_modules_intrinsic_text_initialize(lone);
	lone_modules_intrinsic_list_initialize(lone);
	lone_modules_intrinsic_vector_initialize(lone);
	lone_modules_intrinsic_table_initialize(lone);
}
