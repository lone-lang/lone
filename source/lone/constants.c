/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/constants.h>

#include <lone/struct/lisp.h>

struct lone_value *lone_nil(struct lone_lisp *lone)
{
	return lone->constants.nil;
}

struct lone_value *lone_true(struct lone_lisp *lone)
{
	return lone->constants.truth;
}
