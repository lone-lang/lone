/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/unicode.h>

bool lone_unicode_is_valid_code_point(lone_u32 code_point)
{
	if (code_point > LONE_UNICODE_CODE_POINT_MAX) { return false; }
	if (   code_point >= LONE_UNICODE_SURROGATE_MIN
	    && code_point <= LONE_UNICODE_SURROGATE_MAX) { return false; }

	return true;
}
