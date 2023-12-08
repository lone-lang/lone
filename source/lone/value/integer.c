/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/value/integer.h>

struct lone_value lone_integer_create(long integer)
{
	return (struct lone_value) {
		.type = LONE_INTEGER,
		.as.signed_integer = integer
	};
}

struct lone_value lone_integer_parse(struct lone_lisp *lone, unsigned char *digits, size_t count)
{
	size_t i = 0;
	long integer = 0;

	switch (*digits) { case '+': case '-': ++i; break; }

	while (i < count) {
		integer *= 10;
		integer += digits[i++] - '0';
	}

	if (*digits == '-') { integer *= -1; }

	return lone_integer_create(integer);
}

struct lone_value lone_zero(void)
{
	return lone_integer_create(0);
}

struct lone_value lone_one(void)
{
	return lone_integer_create(+1);
}

struct lone_value lone_minus_one(void)
{
	return lone_integer_create(-1);
}
