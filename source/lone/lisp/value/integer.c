/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/types.h>

#include <lone/linux.h>

struct lone_lisp_value lone_lisp_integer_create(lone_lisp_integer integer)
{
	/* see if integer survives the tag being shifted in and out of it */
	if ((integer << LONE_LISP_DATA_SHIFT) >> LONE_LISP_DATA_SHIFT != integer) {
		/* number is too large for lone's tagged value representation */
		linux_exit(-1);
	}

	return (struct lone_lisp_value) {
		.tagged = (integer << LONE_LISP_DATA_SHIFT) | LONE_LISP_TYPE_INTEGER,
	};
}

struct lone_lisp_value lone_lisp_integer_from_pointer(void *pointer)
{
	return lone_lisp_integer_create((intptr_t) pointer);
}

struct lone_lisp_value lone_lisp_integer_parse(struct lone_lisp *lone, unsigned char *digits, size_t count)
{
	size_t i;
	long integer, digit;
	bool negative;

	i = 0;
	integer = 0;
	negative = false;

	switch (*digits) {
	case '-':
		negative = true;
		__attribute__((fallthrough));
	case '+':
		++i;
		break;
	}

	while (i < count) {
		digit = digits[i++] - '0';
		if (negative) { digit = -digit; }
		if (__builtin_mul_overflow(integer, 10, &integer)) { goto overflow; }
		if (__builtin_add_overflow(integer, digit, &integer)) { goto overflow; }
	}

	return lone_lisp_integer_create(integer);

overflow:
	linux_exit(-1);
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
