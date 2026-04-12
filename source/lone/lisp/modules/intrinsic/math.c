/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/math.h>

#include <lone/lisp/machine.h>
#include <lone/lisp/module.h>
#include <lone/lisp/utilities.h>

#include <lone/linux.h>

void lone_lisp_modules_intrinsic_math_initialize(struct lone_lisp *lone)
{
	struct lone_lisp_value name, module;
	struct lone_lisp_function_flags flags;

	name = lone_lisp_intern_c_string(lone, "math");
	module = lone_lisp_module_for_name(lone, name);
	flags.evaluate_arguments = true;
	flags.evaluate_result = false;

	lone_lisp_module_export_primitive(lone, module, "+",
			"add", lone_lisp_primitive_math_add, module, flags);

	lone_lisp_module_export_primitive(lone, module, "-",
			"subtract", lone_lisp_primitive_math_subtract, module, flags);

	lone_lisp_module_export_primitive(lone, module, "*",
			"multiply", lone_lisp_primitive_math_multiply, module, flags);

	lone_lisp_module_export_primitive(lone, module, "/",
			"divide", lone_lisp_primitive_math_divide, module, flags);

	lone_lisp_module_export_primitive(lone, module, "<",
			"is_less_than", lone_lisp_primitive_math_is_less_than, module, flags);

	lone_lisp_module_export_primitive(lone, module, "<=",
			"is_less_than_or_equal_to", lone_lisp_primitive_math_is_less_than_or_equal_to, module, flags);

	lone_lisp_module_export_primitive(lone, module, ">",
			"is_greater_than", lone_lisp_primitive_math_is_greater_than, module, flags);

	lone_lisp_module_export_primitive(lone, module, ">=",
			"is_greater_than_or_equal_to", lone_lisp_primitive_math_is_greater_than_or_equal_to, module, flags);

	lone_lisp_module_export_primitive(lone, module, "sign",
			"sign", lone_lisp_primitive_math_sign, module, flags);

	lone_lisp_module_export_primitive(lone, module, "zero?",
			"is_zero", lone_lisp_primitive_math_is_zero, module, flags);

	lone_lisp_module_export_primitive(lone, module, "positive?",
			"is_positive", lone_lisp_primitive_math_is_positive, module, flags);

	lone_lisp_module_export_primitive(lone, module, "negative?",
			"is_negative", lone_lisp_primitive_math_is_negative, module, flags);
}

static struct lone_lisp_value lone_lisp_primitive_integer_operation(struct lone_lisp *lone, struct lone_lisp_value arguments, char operation, struct lone_lisp_value accumulator)
{
	struct lone_lisp_value argument;
	lone_lisp_integer x, y, result;

	if (lone_lisp_is_nil(arguments)) {
		/* wasn't given any arguments to operate on: (+), (-), (*) */
		return accumulator;
	}

	do {
		argument = lone_lisp_list_first(lone, arguments);

		switch (lone_lisp_type_of(argument)) {
		case LONE_LISP_TAG_INTEGER:
			switch (lone_lisp_type_of(accumulator)) {
			case LONE_LISP_TAG_INTEGER:
				x = lone_lisp_integer_of(accumulator);
				y = lone_lisp_integer_of(argument);

				switch (operation) {
				case '+':
					if (__builtin_add_overflow(x, y, &result)) { goto overflow; }
					break;
				case '-':
					if (__builtin_sub_overflow(x, y, &result)) { goto overflow; }
					break;
				case '*':
					if (__builtin_mul_overflow(x, y, &result)) { goto overflow; }
					break;
				default:
					/* invalid primitive integer operation */ linux_exit(-1);
				}

				accumulator = lone_lisp_integer_create(result);
				break;
			default:
				/* accumulator is not a number */ linux_exit(-1);
			}
			break;
		default:
			/* argument is not a number */ linux_exit(-1);
		}

		arguments = lone_lisp_list_rest(lone, arguments);

	} while (!lone_lisp_is_nil(arguments));

	return accumulator;

overflow:
	linux_exit(-1);
}

LONE_LISP_PRIMITIVE(math_add)
{
	struct lone_lisp_value arguments, result;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	result = lone_lisp_primitive_integer_operation(lone, arguments, '+', lone_lisp_zero());

	lone_lisp_machine_push_value(lone, machine, result);
	return 0;
}

LONE_LISP_PRIMITIVE(math_subtract)
{
	struct lone_lisp_value arguments, first, accumulator, result;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (!lone_lisp_is_nil(arguments) && !lone_lisp_is_nil(lone_lisp_list_rest(lone, arguments))) {
		/* at least two arguments, set initial value to the first argument: (- 100 58) */
		first = lone_lisp_list_first(lone, arguments);
		if (!lone_lisp_is_integer(lone, first)) { /* argument is not a number */ linux_exit(-1); }
		accumulator = first;
		arguments = lone_lisp_list_rest(lone, arguments);
	} else {
		accumulator = lone_lisp_zero();
	}

	result = lone_lisp_primitive_integer_operation(lone, arguments, '-', accumulator);

	lone_lisp_machine_push_value(lone, machine, result);
	return 0;
}

LONE_LISP_PRIMITIVE(math_multiply)
{
	struct lone_lisp_value arguments, result;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	result = lone_lisp_primitive_integer_operation(lone, arguments, '*', lone_lisp_one());

	lone_lisp_machine_push_value(lone, machine, result);
	return 0;
}

LONE_LISP_PRIMITIVE(math_divide)
{
	struct lone_lisp_value arguments, dividend, divisor, result;
	lone_lisp_integer x, y;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_is_nil(arguments)) { /* at least the dividend is required, (/) is invalid */ goto no_arguments; }
	dividend = lone_lisp_list_first(lone, arguments);
	arguments = lone_lisp_list_rest(lone, arguments);

	switch (lone_lisp_type_of(dividend)) {
	case LONE_LISP_TAG_INTEGER:
		if (lone_lisp_is_nil(arguments)) {
			/* not given a divisor, return 1/x instead: (/ 2) = 1/2 */
			divisor = dividend;
			dividend = lone_lisp_one();
			break;
		} else {
			/* (/ x a b c ...) = x / (a * b * c * ...) */
			divisor = lone_lisp_primitive_integer_operation(lone, arguments, '*', lone_lisp_one());
			break;
		}
	default:
		/* can't divide non-numbers: (/ "not a number") */ goto not_a_number;
	}

	if (lone_lisp_integer_of(divisor) == 0) { /* division by zero: (/ 1 0) */ goto division_by_zero; }

	x = lone_lisp_integer_of(dividend);
	y = lone_lisp_integer_of(divisor);

	/* the only overflowing case for integer division: LONG_MIN / -1 */
	if (x == (-__LONG_MAX__ - 1L) && y == -1) { goto overflow; }

	result = lone_lisp_integer_create(x / y);
	lone_lisp_machine_push_value(lone, machine, result);
	return 0;

overflow:
division_by_zero:
no_arguments:
not_a_number:
		linux_exit(-1);
}

static long apply_and_return(struct lone_lisp *lone, struct lone_lisp_machine *machine,
		lone_lisp_comparator_function comparator)
{
	struct lone_lisp_value arguments, result;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	result = lone_lisp_apply_comparator(lone, arguments, comparator);

	lone_lisp_machine_push_value(lone, machine, result);
	return 0;
}

LONE_LISP_PRIMITIVE(math_is_less_than)
{
	return apply_and_return(lone, machine, lone_lisp_integer_is_less_than);
}

LONE_LISP_PRIMITIVE(math_is_less_than_or_equal_to)
{
	return apply_and_return(lone, machine, lone_lisp_integer_is_less_than_or_equal_to);
}

LONE_LISP_PRIMITIVE(math_is_greater_than)
{
	return apply_and_return(lone, machine, lone_lisp_integer_is_greater_than);
}

LONE_LISP_PRIMITIVE(math_is_greater_than_or_equal_to)
{
	return apply_and_return(lone, machine, lone_lisp_integer_is_greater_than_or_equal_to);
}

static struct lone_lisp_value sign_of(struct lone_lisp_value value)
{
	lone_lisp_integer x = lone_lisp_integer_of(value);

	if (x > 0) {
		return lone_lisp_one();
	} else if (x < 0) {
		return lone_lisp_minus_one();
	} else {
		return lone_lisp_zero();
	}
}

static struct lone_lisp_value compute_sign(struct lone_lisp *lone, struct lone_lisp_machine *machine)
{
	struct lone_lisp_value arguments, value;

	arguments = lone_lisp_machine_pop_value(lone, machine);

	if (lone_lisp_list_destructure(lone, arguments, 1, &value)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	switch (lone_lisp_type_of(value)) {
	case LONE_LISP_TAG_INTEGER:
		return sign_of(value);

	default:
		/* value is not a number */ linux_exit(-1);
	}
}

LONE_LISP_PRIMITIVE(math_sign)
{
	lone_lisp_machine_push_value(lone, machine, compute_sign(lone, machine));
	return 0;
}

static long sign_predicate(struct lone_lisp *lone, struct lone_lisp_machine *machine, struct lone_lisp_value sign)
{
	lone_lisp_integer x, y;

	x = lone_lisp_integer_of(compute_sign(lone, machine));
	y = lone_lisp_integer_of(sign);

	lone_lisp_machine_push_value(lone, machine, lone_lisp_boolean_for(x == y));
	return 0;
}

LONE_LISP_PRIMITIVE(math_is_zero)
{
	return sign_predicate(lone, machine, lone_lisp_zero());
}

LONE_LISP_PRIMITIVE(math_is_positive)
{
	return sign_predicate(lone, machine, lone_lisp_one());
}

LONE_LISP_PRIMITIVE(math_is_negative)
{
	return sign_predicate(lone, machine, lone_lisp_minus_one());
}
