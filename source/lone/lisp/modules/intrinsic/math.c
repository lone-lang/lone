/* SPDX-License-Identifier: AGPL-3.0-or-later */

#include <lone/lisp/modules/intrinsic/math.h>

#include <lone/lisp/value/primitive.h>
#include <lone/lisp/value/list.h>
#include <lone/lisp/value/table.h>
#include <lone/lisp/value/symbol.h>
#include <lone/lisp/value/integer.h>

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

	if (lone_lisp_is_nil(arguments)) {
		/* wasn't given any arguments to operate on: (+), (-), (*) */
		return accumulator;
	}

	do {
		argument = lone_lisp_list_first(arguments);

		switch (argument.type) {
		case LONE_LISP_TYPE_INTEGER:
			switch (accumulator.type) {
			case LONE_LISP_TYPE_INTEGER:
				switch (operation) {
				case '+':
					accumulator.as.integer += argument.as.integer;
					break;
				case '-':
					accumulator.as.integer -= argument.as.integer;
					break;
				case '*':
					accumulator.as.integer *= argument.as.integer;
					break;
				default:
					/* invalid primitive integer operation */ linux_exit(-1);
				}
				break;
			case LONE_LISP_TYPE_NIL:
			case LONE_LISP_TYPE_HEAP_VALUE:
			case LONE_LISP_TYPE_POINTER:
				/* accumulator is not a number */ linux_exit(-1);
			}
			break;
		case LONE_LISP_TYPE_NIL:
		case LONE_LISP_TYPE_HEAP_VALUE:
		case LONE_LISP_TYPE_POINTER:
			/* argument is not a number */ linux_exit(-1);
		}

		arguments = lone_lisp_list_rest(arguments);

	} while (!lone_lisp_is_nil(arguments));

	return accumulator;
}

LONE_LISP_PRIMITIVE(math_add)
{
	return lone_lisp_primitive_integer_operation(lone, arguments, '+', lone_lisp_zero());
}

LONE_LISP_PRIMITIVE(math_subtract)
{
	struct lone_lisp_value first, accumulator;

	if (!lone_lisp_is_nil(arguments) && !lone_lisp_is_nil(lone_lisp_list_rest(arguments))) {
		/* at least two arguments, set initial value to the first argument: (- 100 58) */
		first = lone_lisp_list_first(arguments);
		if (!lone_lisp_is_integer(first)) { /* argument is not a number */ linux_exit(-1); }
		accumulator = first;
		arguments = lone_lisp_list_rest(arguments);
	} else {
		accumulator = lone_lisp_zero();
	}

	return lone_lisp_primitive_integer_operation(lone, arguments, '-', accumulator);
}

LONE_LISP_PRIMITIVE(math_multiply)
{
	return lone_lisp_primitive_integer_operation(lone, arguments, '*', lone_lisp_one());
}

LONE_LISP_PRIMITIVE(math_divide)
{
	struct lone_lisp_value dividend, divisor;

	if (lone_lisp_is_nil(arguments)) { /* at least the dividend is required, (/) is invalid */ linux_exit(-1); }
	dividend = lone_lisp_list_first(arguments);
	arguments = lone_lisp_list_rest(arguments);

	switch (dividend.type) {
	case LONE_LISP_TYPE_INTEGER:
		if (lone_lisp_is_nil(arguments)) {
			/* not given a divisor, return 1/x instead: (/ 2) = 1/2 */
			return lone_lisp_integer_create(1 / dividend.as.integer);
		} else {
			/* (/ x a b c ...) = x / (a * b * c * ...) */
			divisor = lone_lisp_primitive_integer_operation(lone, arguments, '*', lone_lisp_one());
			return lone_lisp_integer_create(dividend.as.integer / divisor.as.integer);
		}
	case LONE_LISP_TYPE_NIL:
	case LONE_LISP_TYPE_POINTER:
	case LONE_LISP_TYPE_HEAP_VALUE:
		/* can't divide non-numbers: (/ "not a number") */ linux_exit(-1);
	}
}

LONE_LISP_PRIMITIVE(math_is_less_than)
{
	return lone_lisp_apply_comparator(lone, arguments, lone_lisp_integer_is_less_than);
}

LONE_LISP_PRIMITIVE(math_is_less_than_or_equal_to)
{
	return lone_lisp_apply_comparator(lone, arguments, lone_lisp_integer_is_less_than_or_equal_to);
}

LONE_LISP_PRIMITIVE(math_is_greater_than)
{
	return lone_lisp_apply_comparator(lone, arguments, lone_lisp_integer_is_greater_than);
}

LONE_LISP_PRIMITIVE(math_is_greater_than_or_equal_to)
{
	return lone_lisp_apply_comparator(lone, arguments, lone_lisp_integer_is_greater_than_or_equal_to);
}

LONE_LISP_PRIMITIVE(math_sign)
{
	struct lone_lisp_value value;

	if (lone_lisp_list_destructure(arguments, 1, &value)) {
		/* wrong number of arguments */ linux_exit(-1);
	}

	switch (value.type) {
	case LONE_LISP_TYPE_INTEGER:
		if (value.as.integer > 0) { return lone_lisp_one(); }
		else if (value.as.integer < 0) { return lone_lisp_minus_one(); }
		else { return lone_lisp_zero(); }
	case LONE_LISP_TYPE_NIL:
	case LONE_LISP_TYPE_POINTER:
	case LONE_LISP_TYPE_HEAP_VALUE:
		/* value is not a number */ linux_exit(-1);
	}
}

LONE_LISP_PRIMITIVE(math_is_zero)
{
	struct lone_lisp_value value = lone_lisp_primitive_math_sign(lone, module, environment, arguments, closure);
	if (value.as.integer == 0) { return value; }
	else { return lone_lisp_nil(); }
}

LONE_LISP_PRIMITIVE(math_is_positive)
{
	struct lone_lisp_value value = lone_lisp_primitive_math_sign(lone, module, environment, arguments, closure);
	if (value.as.integer > 0) { return value; }
	else { return lone_lisp_nil(); }
}

LONE_LISP_PRIMITIVE(math_is_negative)
{
	struct lone_lisp_value value = lone_lisp_primitive_math_sign(lone, module, environment, arguments, closure);
	if (value.as.integer < 0) { return value; }
	else { return lone_lisp_nil(); }
}
