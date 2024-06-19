/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic.h>

#include <lone/lisp/modules/intrinsic/linux.h>
#include <lone/lisp/modules/intrinsic/lone.h>
#include <lone/lisp/modules/intrinsic/math.h>
#include <lone/lisp/modules/intrinsic/bytes.h>
#include <lone/lisp/modules/intrinsic/text.h>
#include <lone/lisp/modules/intrinsic/list.h>
#include <lone/lisp/modules/intrinsic/vector.h>
#include <lone/lisp/modules/intrinsic/table.h>

void lone_lisp_modules_intrinsic_initialize(struct lone_lisp *lone,
		int argc, char **argv, char **envp,
		struct lone_auxiliary_vector *auxv)
{
	lone_lisp_modules_intrinsic_linux_initialize(lone, argc, argv, envp, auxv);
	lone_lisp_modules_intrinsic_lone_initialize(lone);
	lone_lisp_modules_intrinsic_math_initialize(lone);
	lone_lisp_modules_intrinsic_bytes_initialize(lone);
	lone_lisp_modules_intrinsic_text_initialize(lone);
	lone_lisp_modules_intrinsic_list_initialize(lone);
	lone_lisp_modules_intrinsic_vector_initialize(lone);
	lone_lisp_modules_intrinsic_table_initialize(lone);
}
