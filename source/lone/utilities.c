/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/utilities.h>

long lone_min(long x, long y)
{
	if (x <= y) { return x; } else { return y; }
}

long lone_max(long x, long y)
{
	if (x >= y) { return x; } else { return y; }
}
