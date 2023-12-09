/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/constants.h>

struct lone_value lone_true(struct lone_lisp *lone)
{
	return lone->constants.truth;
}
