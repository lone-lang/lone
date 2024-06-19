/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/constants.h>

struct lone_lisp_value lone_lisp_true(struct lone_lisp *lone)
{
	return lone->constants.truth;
}
