/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/value/integer.h>

struct lone_lisp_value lone_lisp_integer_create(lone_lisp_integer integer)
{
	return (struct lone_lisp_value) {
		.type = LONE_LISP_TYPE_INTEGER,
		.as.integer = integer
	};
}

struct lone_lisp_value lone_lisp_integer_parse(struct lone_lisp *lone, unsigned char *digits, size_t count)
{
	size_t i = 0;
	long integer = 0;

	switch (*digits) { case '+': case '-': ++i; break; }

	while (i < count) {
		integer *= 10;
		integer += digits[i++] - '0';
	}

	if (*digits == '-') { integer *= -1; }

	return lone_lisp_integer_create(integer);
}

struct lone_lisp_value lone_lisp_zero(void)
{
	return lone_lisp_integer_create(0);
}

struct lone_lisp_value lone_lisp_one(void)
{
	return lone_lisp_integer_create(+1);
}

struct lone_lisp_value lone_lisp_minus_one(void)
{
	return lone_lisp_integer_create(-1);
}
