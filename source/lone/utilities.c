/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/utilities.h>

#include <limits.h>

long lone_min(long x, long y)
{
	if (x <= y) { return x; } else { return y; }
}

long lone_max(long x, long y)
{
	if (x >= y) { return x; } else { return y; }
}

unsigned long lone_next_power_of_2(unsigned long n)
{
	if (n <= 1) { return 1; }
	return 1UL << ((sizeof(unsigned long) * CHAR_BIT) - __builtin_clzl(n - 1));
}
